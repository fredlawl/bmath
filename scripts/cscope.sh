#/usr/bin/env -S bash -e

outdir=$(readlink -f "$1")
cat compile_commands.json | \
    jq --raw-output0 '.[].file' | \
    xargs -0 -I{} -P4 bash -c 'echo ${1/..\//$2/}' _ {} $outdir $(basename {}) | \
    sort -u > cscope.files

cscope -b -q -k -i cscope.files

cp cscope.files cscope.out "$outdir/" || true
