FROM wiiuenv/devkitppc:20210101

COPY --from=wiiuenv/libmappedmemory:20210109 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20210313 /artifacts $DEVKITPRO

WORKDIR project