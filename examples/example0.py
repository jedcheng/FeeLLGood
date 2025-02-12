import os
import sys

sys.path.insert(0,'../tools')
from settingsMaker import Settings

mySettings = Settings("ellipsoid.msh")
mySettings.createVolRegion( "ellipsoid_volume" )
mySettings.createSurfRegion( "ellipsoid_surface" )

mySettings["Bext"] = [0.01, 0.0, 0.0]
mySettings["outputs"]["evol_columns"] = ["t","<Mx>","<My>","<Mz>","E_tot"]

mySettings.write('mySettings.json')
os.system("../feellgood mySettings.json")
