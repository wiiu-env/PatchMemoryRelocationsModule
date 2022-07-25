FROM wiiuenv/devkitppc:20220724

COPY --from=wiiuenv/libmappedmemory:20220724 /artifacts $DEVKITPRO
COPY --from=wiiuenv/wiiumodulesystem:20220724 /artifacts $DEVKITPRO

WORKDIR project