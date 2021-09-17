FROM wiiuenv/devkitppc:20210917

COPY --from=wiiuenv/libmappedmemory:20210109 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20210917 /artifacts $DEVKITPRO

WORKDIR project