# LibTandy sources used by NyanCat

This directory vendors the stable LTSCORE/1 subset for the NyanCat Tandy adapter. The pinned source snapshot was exported from `astrobleem/libtandy` at base commit `3335a48fb8aab646c89a790ab0a9909d1b1dfb07` as `libtandy-ltscore-nyan-subset-20261022.zip` (SHA-256 `4493B8EE9184C75F51142FC8323B07F9926F5C35664811FF4AA3EB0A56F720DF`). `MANIFEST-SHA256.txt` records each vendored file hash.

The library subset is MIT licensed; see `LICENSE`. `src/lt_dos.c` is a NyanCat-specific hardware output adapter and is not part of the library export. Nyan's score tables remain in `SRC/TANDY.C`. This directory does not include JAWS sample data or LTSFX interoperability modules.
