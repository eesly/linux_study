set helplang=cn
set encoding=utf-8

syntax enable

if has('gui_running')
    set background=light
else
    set background=dark
endif

" 显示tab和空格
set list
set lcs=tab:>-,trail:-,extends:>,precedes:<,nbsp:.
highlight LeaderTab guifg=#666666
match LeaderTab /^\t/

" 显示行号
set nu

" 设置tab = 4个空格
set ts=4
set expandtab

" colorscheme solarized

" 设置状态栏
if has('statusline')
    set laststatus=2
    set statusline=%<%f\   " 文件名
    set statusline+=%w%h%m%r " 选项
    set statusline+=\ [%{&ff}/%Y]            " filetype
    set statusline+=\ [%{getcwd()}]          " current dir
    set statusline+=\ [A=\%03.3b/H=\%02.2B] " ASCII / Hexadecimal value of char
    set statusline+=%=%-14.(%l,%c%V%)\ %p%%\ %L  " Right aligned file nav info
endif

