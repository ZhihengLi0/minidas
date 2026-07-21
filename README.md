# minidas

C++ package to reduce SuperCDMS/CUTE raw data volume on MSI.

At SLAC, raw MIDAS data (~TB) is processed by **cdmsbats** into RQ ROOT
files (~MB). Both are copied to MSI. `minidas` then provides two
subcommands in a single executable:

1. **`minidas eventlist`** — applies a *very loose* preselection to a
   processed RQ file (drops the clearly unusable ~1–5%, i.e. keeps only
   events passing basic cuts) and writes an **eventlist** CSV
   (`EventNumber,DumpNumber,EventTime,EventType,PTOFamps`).
2. **`minidas skim`** — reads the raw MIDAS dumps and the eventlist and
   writes a single **selected raw data** file, still in MIDAS format
   (ODB copied from the first dump, so the output is self-contained).

## Dependencies

- **CDMSIOLIB** (`libcdmsio.so`) and **ROOT**: both are shipped inside
  the `cdmsfull_*.sif` singularity images
  (`/projects/standard/yanliusp/shared/singularity_images/` on MSI); the
  image's CDMSIOLIB lives at `/opt/cdms/release` and is picked up by the
  Makefile automatically. Outside the image, the Makefile falls back to
  `/projects/standard/yanliusp/shared/analyses/Max/IOLibrary` (needs a
  ROOT version matching how that copy was built; override with
  `make CDMSIOLIB=...`).

## Build

Build (and run) inside a cdmsfull image:

```sh
cd /users/9/li004628/minidas
singularity exec -B /projects \
    /projects/standard/yanliusp/shared/singularity_images/cdmsfull_V07-02-00.sif make
```

Note: the container only auto-binds your cwd and home — pass
`-B /projects` so data and shared software paths are visible inside.

## Run

```sh
./minidas eventlist -i CUTE_..._23231217_135018.root -o eventlist.csv \
    --amp-min 4e-6 --amp-max 9e-6 --type 1 --zip zip1

./minidas skim -e eventlist.csv \
    -i /path/to/Raw/24260617_063934/24260617_063934_ \
    -o selected_24260617_063934.mid.gz
```

`-i` for `minidas skim` is the series prefix: dump *N* is read from
`<prefix>F%04d.mid.gz` (`.mid.lz4` and plain `.mid` are auto-detected).

Long jobs (a full series is many dumps) should be submitted via SLURM;
see `scripts/skim.sbatch`.

## Notes / assumptions

- Event-number matching does not hardcode any numbering convention:
  `minidas skim` calls `MidasFileReader::SetDumpNumber(dump)` so
  CDMSIOLIB assigns each raw event the same Soudan-style number
  (`10000*dump + index_in_dump`) that cdmsbats assigned when producing
  the RQ file. Matching is done per dump, so it stays correct even if a
  dump does not contain exactly 10000 events.
- `eventTree` and the `zipN` tree in the RQ file are assumed parallel
  (one entry per event, same order); this is validated at startup.
- Per-dump RQ files carry no `DumpNumber` branch; the dump is then
  derived from the Soudan-style `EventNumber`.
- `legacy/` holds the original prototypes this package was built from
  (`AddSalt.cxx`, `extract_waveform.cpp`, `Filter.ipynb` from
  `software/cdms-waveform`).
