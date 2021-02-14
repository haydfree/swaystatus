let g:ale_linter_aliases = {'h': 'c'}

let include_path = system('pkg-config --cflags upower-glib alsa')

let g:ale_c_gcc_options = "-std=c11 " . expand(include_path)
let g:ale_c_clang_options = "-std=c11 " . expand(include_path)

let g:ale_c_clangtidy_options = "-std=c11 " . expand(include_path)
let g:ale_c_clangcheck_options = "-std=c11 " . expand(include_path)
let g:ale_c_clangd_options = "-std=c11 " . expand(include_path)
