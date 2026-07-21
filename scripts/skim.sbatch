#!/bin/bash
#SBATCH --job-name=rawskim
#SBATCH --time=04:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=8g
#SBATCH -A yanliusp
#SBATCH -o %x_%j.out

# Edit these paths, then: sbatch skim.sbatch
EVENTLIST=eventlist.csv
RAW_PREFIX=/path/to/Raw/23231222_074513/23231222_074513_
OUTPUT=selected.mid.gz

SIF=/projects/standard/yanliusp/shared/singularity_images/cdmsfull_V07-02-00.sif
PKG=/projects/standard/yanliusp/shared/software/cdms-rawskim

# The binaries are linked against libraries inside the image, so they
# must run inside it; -B /projects makes data paths visible there.
singularity exec -B /projects "$SIF" \
    "$PKG/skim_raw.exe" -q -e "$EVENTLIST" -i "$RAW_PREFIX" -o "$OUTPUT"
