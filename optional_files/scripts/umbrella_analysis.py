### script to analyse an umbrella calculation on ECPC ###

import os
import shutil

# USER INPUT
STEPS = range(-180, 185, 5)           # to which values should the restraint be set? 
FORCE_CONSTANT = 0.05                 # force constant of biasing potential
PATH = "/apps/wham/wham/wham"         # path to WHAM program

# ... for WHAM
REACTION_COORD = "torsion"   # what kind of reaction coordinate? 'bond' or 'torsion'
MIN = -182.5                 # minimum boundary of histogram
MAX = 182.5                  # maximum boundary of histogram
BINS = 73                    # number of points in final PMF (= number of bins)
TEMP = 300                   # temperature for WHAM 
TOL = 0.00001                # tolerance for WHAM interations


# creates a folder 'analysis' and copies all 'umbrella.txt' files there
# to the names of the files the step is added
# furthermore a file 'input.txt' is created that is used as input for WHAM
# if files are missing this function returns false, otherwise true
def copy_and_and_create_inputfile():
    success = True
    os.mkdir("analysis") # create folder
    for s in STEPS:
        if os.path.isfile("{}/umbrella.txt".format(s)):
            # copy and rename file
            shutil.copy("{}/umbrella.txt".format(s),"analysis/umbrella_{}.txt".format(s))
            # write inputfile for WHAM
            with open("analysis/in.txt","a") as inp:         
                inp.write("umbrella_{}.txt   {}    {}\n".format(s,float(s),FORCE_CONSTANT))
        else:
            print "No file 'umbrella.txt' in folder", s
            sucess = False
    return success

if copy_and_and_create_inputfile():
    os.chdir("analysis")
    if REACTION_COORD == "bond":
        os.popen("{} {} {} {} {:.9f} {} 0 in.txt out.txt".format(PATH,MIN,MAX,BINS,TOL,TEMP))
    elif REACTION_COORD == "torsion":
        os.popen("{} P {} {} {} {:.9f} {} 0 in.txt out.txt".format(PATH,MIN,MAX,BINS,TOL,TEMP))
    else:
        print "ERROR: unvalid kind of reaction coordinate!"
