# Build: docker build -o . .
FROM devkitpro/devkitppc:20260117 AS builder

COPY . /app
WORKDIR /app
RUN make -j$(nproc)

FROM scratch AS export-stage
COPY --from=builder /app/boot.dol /
COPY --from=builder /app/boot.elf /
