FROM wiiuenv/devkitppc:20211229

COPY --from=wiiuenv/libmappedmemory:20210924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220123 /artifacts $DEVKITPRO

WORKDIR project