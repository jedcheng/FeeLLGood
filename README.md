# FeeLLGood – A micromagnetic solver ![Build Status](https://github.com/feellgood/FeeLLGood/actions/workflows/tests.yml/badge.svg)

FEELLGOOD is a micromagnetic solver using finite element technique to integrate Landau Lifshitz Gilbert equation. It computes the demagnetizing field using the so-called fast multipole algorithm.

It is developped by JC Toussaint & al.
The code is being modified without any warranty it works. A dedicated website can be found [here][]  

### Dependencies

* C++17 and the STL
* [TBB][]
* [yaml-cpp][]
* [ANN][] 1.1.2
* [exprtk][] revision [806c519c91][exprtk-rev] (it should also work with the latest release)
* [ScalFMM][] revision [22b9e4f6cf][ScalFMM-rev] (it should also work with V1.5.1)
* [GMM][] 5.4 (it should also work with 4.x)

[here]: https://feellgood.neel.cnrs.fr/
[TBB]: https://www.threadingbuildingblocks.org/
[yaml-cpp]: https://github.com/jbeder/yaml-cpp
[ANN]: https://www.cs.umd.edu/~mount/ANN/
[exprtk]: https://www.partow.net/programming/exprtk/index.html
[exprtk-rev]: https://github.com/ArashPartow/exprtk/archive/806c519c91fd08ba4fa19380dbf3f6e42de9e2d1.zip
[ScalFMM]: https://gitlab.inria.fr/solverstack/ScalFMM/
[ScalFMM-rev]: https://gitlab.inria.fr/solverstack/ScalFMM/-/archive/22b9e4f6cf4ea721d71198a71e3f5d2c5ae5e7cc/ScalFMM-22b9e4f6cf4ea721d71198a71e3f5d2c5ae5e7cc.tar.gz
[GMM]: http://www.getfem.org/gmm/
