FROM ghcr.io/wiiu-env/devkitppc:20260225

COPY --from=ghcr.io/wiiu-env/libmappedmemory:20260331 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiumodulesystem:reentfix-dev-20260403-5ca1144 /artifacts $DEVKITPRO

WORKDIR project
