#!/usr/bin/env python3

from graphTools import *
from expTools import *
import os

# Dictionnaire avec les options de compilations d'apres commande

### Partie OpenCL
options = {}
options["-k "] = ["sable"]
options["-i "] = [1000]
options["-o "] = [""]
options["-v "] = ["ocl_mine"]
options["-s "] = [480, 1024, 2048, 4096]
# Pour renseigner l'option '-of' il faut donner le chemin depuis le fichier easypap
options["-of "] = ["./plots/data/perf_data.csv"]

ompenv = {}
ompenv["TILEX="] = [16]
ompenv["TILEY="] = [16]

nbrun = 2
# Lancement des experiences
execute('./run ', ompenv, options, nbrun, verbose=False, easyPath=".")

ompenv["TILEX="] = [32]
ompenv["TILEY="] = [32]
execute('./run ', ompenv, options, nbrun, verbose=False, easyPath=".")

ompenv["TILEX="] = [16]
ompenv["TILEY="] = [32]
execute('./run ', ompenv, options, nbrun, verbose=False, easyPath=".")

ompenv["TILEX="] = [32]
ompenv["TILEY="] = [16]
execute('./run ', ompenv, options, nbrun, verbose=False, easyPath=".")
