/**
  Elementary matrix Calculation for a tetrahedron element
 */

#include <set>

#include "config.h"  // to get gamma0 constant

#include "pt3D.h"
#include "tetra.h"
#include "time_integration.h"
#include "tiny.h"

#include "facette.h"

using namespace Tetra;
using namespace Pt;

void Tet::lumping(int const &npi, double alpha_eff, double prefactor,
                  double (&AE)[3 * N][3 * N]) const
    {
    const double w = weight[npi];

    for (int i = 0; i < N; i++)
        {
        const double ai_w = w * a[i][npi];
        const pt3D ai_w_u0 = ai_w * Nodes::get_u0(refNode[ind[i]]);

        AE[i][i] += alpha_eff * ai_w;
        AE[N + i][N + i] += alpha_eff * ai_w;
        AE[2 * N + i][2 * N + i] += alpha_eff * ai_w;

        AE[0 * N + i][2 * N + i] += ai_w_u0(Pt::IDX_Y);
        AE[0 * N + i][1 * N + i] -= ai_w_u0(Pt::IDX_Z);
        AE[1 * N + i][0 * N + i] += ai_w_u0(Pt::IDX_Z);
        AE[1 * N + i][2 * N + i] -= ai_w_u0(Pt::IDX_X);
        AE[2 * N + i][1 * N + i] += ai_w_u0(Pt::IDX_X);
        AE[2 * N + i][0 * N + i] -= ai_w_u0(Pt::IDX_Y);

        for (int j = 0; j < N; j++)
            {
            double contrib = w * prefactor
                             * (dadx[i][npi] * dadx[j][npi] + dady[i][npi] * dady[j][npi]
                                + dadz[i][npi] * dadz[j][npi]);

            AE[i][j] += contrib;
            AE[N + i][N + j] += contrib;
            AE[2 * N + i][2 * N + j] += contrib;
            }
        }
    }

void Tet::add_drift_BE(int const &npi, double alpha, double s_dt, double Vdrift, Pt::pt3D (&U)[NPI],
                       Pt::pt3D (&V)[NPI], Pt::pt3D (&dUd_)[NPI], Pt::pt3D (&dVd_)[NPI],
                       Pt::pt3D (&BE)[N]) const
    {  // the artificial drift from eventual recentering is along x,y or z
    double w = weight[npi];

    for (int i = 0; i < N; i++)
        {
        const double ai_w = w * a[i][npi];
        BE[i] += ai_w * Vdrift * (alpha * dUd_[npi] + U[npi] * dUd_[npi]);
        BE[i] += (ai_w * s_dt) * Vdrift
                 * (alpha * dVd_[npi] + U[npi] * dVd_[npi] + V[npi] * dUd_[npi]);
        }
    }

void Tet::build_BE(int const &npi, Pt::pt3D const &H, double Abis, Pt::pt3D (&dUdx)[NPI],
                   Pt::pt3D (&dUdy)[NPI], Pt::pt3D (&dUdz)[NPI], Pt::pt3D (&BE)[N]) const
    {
    const double w = weight[npi];

    for (int i = 0; i < N; i++)
        {
        BE[i] -= (w * Abis)
                 * (dadx[i][npi] * dUdx[npi] + dady[i][npi] * dUdy[npi] + dadz[i][npi] * dUdz[npi]);
        BE[i] += (w * a[i][npi]) * H;
        }
    }

double Tet::calc_aniso_uniax(int const &npi, Pt::pt3D const &uk, const double Kbis,
                             const double s_dt, Pt::pt3D (&U)[NPI], Pt::pt3D (&V)[NPI],
                             Pt::pt3D &H_aniso) const
    {
    H_aniso += (Kbis * pScal(uk, U[npi] + s_dt * V[npi])) * uk;
    return (Kbis * sq(pScal(uk, U[npi])));
    }

double Tet::calc_aniso_cub(int const &npi, Pt::pt3D const &ex, Pt::pt3D const &ey,
                           Pt::pt3D const &ez, const double K3bis, const double s_dt,
                           Pt::pt3D (&U)[NPI], Pt::pt3D (&V)[NPI], Pt::pt3D &H_aniso) const
    {
    pt3D uk_u = pt3D(pScal(ex, U[npi]), pScal(ey, U[npi]), pScal(ez, U[npi]));
    pt3D uk_v = pt3D(pScal(ex, V[npi]), pScal(ey, V[npi]), pScal(ez, V[npi]));
    Pt::pt3D cube_uk_u = directCube(uk_u);
    Pt::pt3D uk_uuu = uk_u - cube_uk_u;

    H_aniso += -K3bis
               * (uk_uuu(0) * ex + uk_uuu(1) * ey + uk_uuu(2) * ez
                  + s_dt * pDirect(pDirect(uk_v, ex), pt3D(1, 1, 1) - 3 * pDirect(uk_u, uk_u)));
    return (-K3bis * pScal(uk_u, uk_uuu));
    }

void Tet::integrales(std::vector<Tetra::prm> const &params, timing const &prm_t,
                     Pt::pt3D const &Hext, Pt::index idx_dir, double Vdrift,
                     double (&AE)[3 * N][3 * N], Pt::pt3D (&BE)[N]) const
    {
    double alpha = params[idxPrm].alpha_LLG;
    double Js = params[idxPrm].J;
    double Abis = 2.0 * (params[idxPrm].A) / Js;
    double Kbis = 2.0 * (params[idxPrm].K) / Js;
    double K3bis = 2.0 * (params[idxPrm].K3) / Js;
    const double s_dt = THETA * prm_t.get_dt() * gamma0;  // theta from theta scheme in config.h.in

    /*-------------------- INTERPOLATION --------------------*/
    pt3D Hd[NPI], dUdx[NPI], dUdy[NPI], dUdz[NPI];
    pt3D Hv[NPI], dVdx[NPI], dVdy[NPI], dVdz[NPI];
    pt3D U[NPI], V[NPI];

    interpolation(Nodes::get_u0, U, dUdx, dUdy, dUdz);
    interpolation(Nodes::get_v0, V, dVdx, dVdy, dVdz);
    interpolation(Nodes::get_phi0, Hd);
    interpolation(Nodes::get_phiv0, Hv);

    for (int npi = 0; npi < NPI; npi++)
        {
        pt3D H_aniso;
        double contrib_aniso(0);

        contrib_aniso += calc_aniso_uniax(npi, params[idxPrm].uk, Kbis, s_dt, U, V, H_aniso);
        contrib_aniso += calc_aniso_cub(npi, params[idxPrm].ex, params[idxPrm].ey,
                                        params[idxPrm].ez, K3bis, s_dt, U, V, H_aniso);

        pt3D H = H_aniso + Hd[npi] + Hext + (s_dt / gamma0) * Hv[npi];
        build_BE(npi, H, Abis, dUdx, dUdy, dUdz, BE);
        extraCoeffs_BE(npi, Js, U[npi], dUdx[npi], dUdy[npi], dUdz[npi], BE);  // STT

        if (idx_dir != Pt::IDX_UNDEF)
            {
            if (idx_dir == Pt::IDX_Z)
                add_drift_BE(npi, alpha, s_dt, Vdrift, U, V, dUdz, dVdz, BE);
            else if (idx_dir == Pt::IDX_Y)
                add_drift_BE(npi, alpha, s_dt, Vdrift, U, V, dUdy, dVdy, BE);
            else if (idx_dir == Pt::IDX_X)
                add_drift_BE(npi, alpha, s_dt, Vdrift, U, V, dUdx, dVdx, BE);
            }

        H = Hext + Hd[npi];
        extraField(npi, H);  // add STT contribution to the effective field H
        double uHeff =
                contrib_aniso - Abis * (norme2(dUdx[npi]) + norme2(dUdy[npi]) + norme2(dUdz[npi]));
        uHeff += pScal(U[npi], H);

        lumping(npi, prm_t.calc_alpha_eff(alpha, uHeff), prm_t.prefactor * s_dt * Abis, AE);
        }
    }

double Tet::exchangeEnergy(Tetra::prm const &param, const double (&dudx)[DIM][NPI],
                           const double (&dudy)[DIM][NPI], const double (&dudz)[DIM][NPI]) const
    {
    double dens[NPI];

    for (int npi = 0; npi < NPI; npi++)
        {
        dens[npi] = sq(dudx[0][npi]) + sq(dudy[0][npi]) + sq(dudz[0][npi]) + sq(dudx[1][npi])
                    + sq(dudy[1][npi]) + sq(dudz[1][npi]) + sq(dudx[2][npi]) + sq(dudy[2][npi])
                    + sq(dudz[2][npi]);
        }
    return (param.A * weightedScalarProd(dens));
    }

double Tet::anisotropyEnergy(Tetra::prm const &param, const double (&u)[DIM][NPI]) const
    {
    double dens[NPI];

    for (int npi = 0; npi < NPI; npi++)
        {
        Pt::pt3D m = Pt::pt3D(u[0][npi], u[1][npi], u[2][npi]);
        // uniaxial magnetocrystalline anisotropy constant K, anisotropy axis uk
        dens[npi] = -param.K * sq(Pt::pScal(param.uk, m));

        // cosinus directeurs
        double al0 = Pt::pScal(param.ex, m);
        double al1 = Pt::pScal(param.ey, m);
        double al2 = Pt::pScal(param.ez, m);

        dens[npi] += param.K3
                     * (sq(al0 * al1) + sq(al1 * al2) + sq(al2 * al0));  // cubic anisotropy (K3)
        }

    return weightedScalarProd(dens);
    }

void Tet::charges(std::function<Pt::pt3D(Nodes::Node)> getter, std::vector<double> &srcDen,
                  int &nsrc, double Ms) const
    {
    double dudx[DIM][NPI], dudy[DIM][NPI], dudz[DIM][NPI];
    interpolation(getter, dudx, dudy, dudz);

    for (int j = 0; j < NPI; j++, nsrc++)
        {
        srcDen[nsrc] = -Ms * (dudx[0][j] + dudy[1][j] + dudz[2][j]) * weight[j];
        }
    }

double Tet::demagEnergy(Tetra::prm const &param, const double (&dudx)[DIM][NPI],
                        const double (&dudy)[DIM][NPI], const double (&dudz)[DIM][NPI],
                        const double (&phi)[NPI]) const
    {
    double dens[NPI];
    double Ms = nu0 * param.J;

    for (int npi = 0; npi < NPI; npi++)
        {
        dens[npi] = (dudx[0][npi] + dudy[1][npi] + dudz[2][npi]) * phi[npi];
        }
    return (-0.5 * mu0 * Ms * weightedScalarProd(dens));
    }

double Tet::zeemanEnergy(Tetra::prm const &param, double uz_drift, Pt::pt3D const &Hext,
                         double const (&u)[DIM][NPI]) const
    {
    double dens[NPI];

    for (int npi = 0; npi < NPI; npi++)
        {
        Pt::pt3D u_npi = Pt::pt3D(u[0][npi], u[1][npi], u[2][npi]);
        dens[npi] = Pt::pScal(u_npi, Hext) + uz_drift * Hext(Pt::IDX_Z);
        }
    return (-param.J * weightedScalarProd(dens));
    }

void Tet::assemblage_mat(write_matrix &K, const int offset) const
    {
    for (int i = 0; i < N; i++)
        {
        int i_ = ind[i];

        for (int j = 0; j < N; j++)
            {
            int j_ = ind[j];
            K(offset + i_, j_) += Kp[i][j];
            K(offset + i_, offset + j_) += Kp[i][N + j];
            K(i_, j_) += Kp[N + i][j];
            K(i_, offset + j_) += Kp[N + i][N + j];
            }
        }
    }

double Tet::Jacobian(double (&J)[DIM][DIM])
    {
    Pt::pt3D const &p0 = refNode[ind[0]].p;
    Pt::pt3D const &p1 = refNode[ind[1]].p;
    Pt::pt3D const &p2 = refNode[ind[2]].p;
    Pt::pt3D const &p3 = refNode[ind[3]].p;
    J[0][0] = p1.x() - p0.x();
    J[0][1] = p2.x() - p0.x();
    J[0][2] = p3.x() - p0.x();
    J[1][0] = p1.y() - p0.y();
    J[1][1] = p2.y() - p0.y();
    J[1][2] = p3.y() - p0.y();
    J[2][0] = p1.z() - p0.z();
    J[2][1] = p2.z() - p0.z();
    J[2][2] = p3.z() - p0.z();

    return Pt::det(J);
    }

double Tet::calc_vol(void) const
    {
    Pt::pt3D const &p0 = refNode[ind[0]].p;
    Pt::pt3D const &p1 = refNode[ind[1]].p;
    Pt::pt3D const &p2 = refNode[ind[2]].p;
    Pt::pt3D const &p3 = refNode[ind[3]].p;

    return Pt::pTriple(p1 - p0, p2 - p0, p3 - p0) / 6.0;
    }

std::set<Facette::Fac> Tet::ownedFac() const
    {
    std::set<Facette::Fac> s;

    const int ia = ind[0];
    const int ib = ind[1];
    const int ic = ind[2];
    const int id = ind[3];

    s.insert(Facette::Fac(refNode, 0, idxPrm, ia, ic, ib));
    s.insert(Facette::Fac(refNode, 0, idxPrm, ib, ic, id));
    s.insert(Facette::Fac(refNode, 0, idxPrm, ia, id, ic));
    s.insert(Facette::Fac(refNode, 0, idxPrm, ia, ib, id));

    return s;
    }
