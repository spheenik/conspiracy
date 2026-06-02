#!/bin/bash
# Post-process the objconv-converted mvx ELF archive ($1 -> $2) so it links WITHOUT
# -Wl,--allow-multiple-definition. That flag's order-dependent "first definition wins"
# silently mis-wired per-plugin methods when link order changed (e.g. the Drum plugin
# ran another plugin's ?Tick -> no percussion). Two distinct problems from the COFF->ELF
# conversion of the MSVC lib, both fixed deterministically here:
#
#  1) MSVC mangled names contain '@@', which ELF `ld` parses as symbol VERSIONING
#     (name@@version). So ?Tick@mvxPlugDrum@@UAEXXZ, ?Tick@mvxPlugDelay@@UAEXXZ, ...
#     all collapse to base symbol `?Tick` -> false "multiple definition", resolved by
#     link order. Fix: replace every '@' with '_' (consistently across definitions AND
#     references) so the distinct mangled names stay distinct ordinary symbols.
#
#  2) MSVC COMDAT "select any" data -- FP constant pools (__real_/__xmm_) and C++
#     vtables (??_*) -- is emitted as *strong* duplicates (ELF has no select-any). These
#     copies are byte-identical; weaken them so the linker folds a single copy.
set -e
IN="$1"; OUT="$2"
cp -f "$IN" "$OUT"

# 1) de-version: strip '@' from all symbol names
nm "$OUT" | awk '{print $NF}' | grep '@' | sort -u | \
  awk '{o=$0; n=$0; gsub(/@/,"_",n); print o, n}' > "$OUT.redef"
objcopy --redefine-syms="$OUT.redef" "$OUT"
rm -f "$OUT.redef"

# 2) fold identical COMDAT data
objcopy --wildcard --weaken-symbol='__real_*' --weaken-symbol='__xmm_*' --weaken-symbol='??_*' "$OUT"
