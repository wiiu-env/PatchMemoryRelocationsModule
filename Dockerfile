FROM wiiuenv/devkitppc:20220507

COPY --from=wiiuenv/libmappedmemory:20220211 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220512 /artifacts $DEVKITPRO

WORKDIR project