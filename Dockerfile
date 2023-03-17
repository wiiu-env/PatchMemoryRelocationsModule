FROM ghcr.io/wiiu-env/devkitppc:20220917

COPY --from=ghcr.io/wiiu-env/libmappedmemory:20220904 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:20221005 /artifacts $DEVKITPRO

WORKDIR project