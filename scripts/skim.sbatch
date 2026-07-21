#!/bin/bash
#SBATCH --job-name=rawskim
#SBATCH --time=04:00:00
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem=8g
#SBATCH -A yanliusp
#SBATCH -o %x_%j.out

# Edit these three paths, then: sbatch skim.sbatch
EVENTLIST=eventlist.csv
RAW_PREFIX=/path/to/Raw/23231222_074513/23231222_074513_
OUTPUT=selected.mid.gz

cd /projects/standard/yanliusp/shared/software/cdms-rawskim
./skim_raw.exe -q -e "$EVENTLIST" -i "$RAW_PREFIX" -o "$OUTPUT"
