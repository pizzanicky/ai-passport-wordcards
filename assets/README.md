# Asset provenance and regeneration

`data/words.json`: 3,676 ECDICT `gk` candidates. Source hash, exclusions, normalization and unreviewed status are in `data/corpus-review.json`. Selection and normalized IPA do not guarantee syllabus coverage, common-meaning correctness or phonetic accuracy. All entries currently have `reviewed: false`.

`music/words.mp3`: locally generated Piper en_GB-alba-medium, mono 16 kHz, 16 kbit/s MP3. Offsets/lengths are in each word record. Manifest records engine/model/audio hashes and explicit NOT_AUDITED listening status. Model card and license records are in `music/`. Model/dataset/fine-tuning license compatibility remains a release gate.

`fonts/`: Noto Sans and Noto Sans SC, OFL licenses retained. The 20 px definition font uses 2-bit glyphs with lossless compression to preserve flash reserve; remaining sizes use 4-bit glyphs. Font sources are fetched into ignored cache. Generated character subsets cover application text and corpus IPA/CJK, including stress and length marks.

`images/character-source.png`: adaptation of the user's daughter's original character, approved for this UI. No reference-image text is included. `wc_portraits.c` contains compact RGB565 portraits. Public distribution authorization and ownership credit must be confirmed before public release.

Regeneration order: `tools/fetch_sources.py`, `tools/prepare_corpus.py`, `tools/generate_audio.py`, `tools/generate_assets.py`. Python dependencies are in `tools/requirements-assets.txt`; install font converter with `npm ci --prefix tools`. Audio generation requires FFmpeg and is local; fetching dependencies/models requires network. Regenerating the corpus removes packed audio offsets; repack audio before regenerating C assets. Revalidate after changes.
