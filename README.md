# cdms-rawskim

C++ package to reduce SuperCDMS/CUTE raw data volume on MSI.

At SLAC, raw MIDAS data (~TB) is processed by **cdmsbats** into RQ ROOT
files (~MB). Both are copied to MSI. This package then:

1. **`make_eventlist.exe`** — applies a *very loose* preselection to a
   processed RQ file (drops the clearly unusable ~1–5% ... i.e. keeps only
   events passing basic cuts) and writes an **eventlist** CSV
   (`EventNumber,DumpNumber,EventTime,EventType,PTOFamps`).
2. **`skim_raw.exe`** — reads the raw MIDAS dumps and the eventlist and
   writes a single **selected raw data** file, still in MIDAS format
   (ODB copied from the first dump, so the output is self-contained).

## Dependencies

- **CDMSIOLIB** (`libcdmsio.so`) and **ROOT**: both are shipped inside the
  `cdmsfull_*.sif` singularity images
  (`/projects/standard/yanliusp/shared/singularity_images/` on MSI); the
  image's CDMSIOLIB lives at `/opt/cdms/release` and is picked up by the
  Makefile automatically. Outside the image, the Makefile falls back to
  `/projects/standard/yanliusp/shared/analyses/Max/IOLibrary` (needs a
  ROOT version matching how that copy was built; override with
  `make CDMSIOLIB=...`).

## Build

Build (and run) inside a cdmsfull image:

```sh
cd /projects/standard/yanliusp/shared/software/cdms-rawskim
singularity exec -B /projects \
    /projects/standard/yanliusp/shared/singularity_images/cdmsfull_V07-02-00.sif make
```

Note: the container only auto-binds your cwd and home — pass
`-B /projects` so data and shared software paths are visible inside.

## Run

```sh
./make_eventlist.exe -i CUTE_..._23231217_135018.root -o eventlist.csv \
    --amp-min 4e-6 --amp-max 9e-6 --type 1

./skim_raw.exe -e eventlist.csv \
    -i /path/to/Raw/23231222_074513/23231222_074513_ \
    -o selected_23231222_074513.mid.gz
```

`-i` for `skim_raw.exe` is the series prefix: dump *N* is read from
`<prefix>F000N.mid.gz`.

Long jobs (a full series is many dumps) should be submitted via SLURM;
see `scripts/skim.sbatch`.

## Notes / assumptions

- Global-vs-local event number mapping (inherited from the original
  `AddSalt.cxx`): `global = local + DumpNumber*10000 - 10000`, i.e.
  10000 events per dump. If a run uses a different dump size, this must
  be adapted.
- `eventTree` and the `zipN` tree in the RQ file are assumed parallel
  (one entry per event, same order).
- `legacy/` holds the original prototypes this package was built from
  (`AddSalt.cxx`, `extract_waveform.cpp`, `Filter.ipynb` from
  `software/cdms-waveform`).
