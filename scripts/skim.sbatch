#!/bin/bash
#SBATCH --job-name=minidas-skim
#SBATCH --time=04:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=8g
#SBATCH -A yanliusp
#SBATCH -o %x_%j.out

# Edit these paths, then: sbatch skim.sbatch
EVENTLIST=eventlist.csv
RAW_PREFIX=/path/to/Raw/24260617_063934/24260617_063934_
OUTPUT=selected.mid.gz

SIF=/projects/standard/yanliusp/shared/singularity_images/cdmsfull_V07-02-00.sif
PKG=/users/9/li004628/minidas

# The binary is linked against libraries inside the image, so it must
# run inside it; -B /projects makes data paths visible there.
singularity exec -B /projects "$SIF" \
    "$PKG/minidas" skim -q -e "$EVENTLIST" -i "$RAW_PREFIX" -o "$OUTPUT"
