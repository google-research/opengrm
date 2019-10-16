" Vim syntax for Thrax grammar source files.
" To use this, put this s/where easily accessible (maybe in $VIM/syntax/ or,
" more likely, in ~/.vim/syntax/) and then add the following to your .vimrc:
"   au BufRead,BufNewFile *.grm set filetype=thraxgrm
" If you choose a custom location, then you can specify the location via:
"   au! Syntax thraxgrm source /path/to/thraxgrm.vim

if exists("b:current_syntax")
  finish
endif

syn keyword thraxKeyword as import return
syn keyword thraxParseKeyword byte utf8
syn keyword thraxIncludedKeyword export func contained
syn keyword thraxBuiltinFunctions Analyzer ArcSort AssertEmpty AssertEqual AssertNull CDRewrite Category Closure Compose Concat Determinize Difference Expand Feature FeatureVector Invert LenientlyCompose LoadFst LoadFstFromFar Minimize MPdtCompose Optimize ParadigmReplace PdtCompose Project Replace Reverse Rewrite RmEpsilon RmWeight StringFile StringFst SymbolTable Tagger Union

syn match   thraxSymbols "\(\[\|\]\|=\|{\|}\|;\||\|+\|\*\|-\|,\|?\|<\|>\|:\|@\|(\|)\)" display
syn match   thraxBackslashedChar "\\." display contained
syn match   thraxComment        "#.*$"
syn region  thraxFstString    start=/"/ skip=/\\"/ end=/"/ contains=thraxBackslashedChar
syn region  thraxLiteralString    start=/'/ skip=/\\'/ end=/'/ contains=thraxBackslashedChar
syn region  thraxLiteralString    start=/</ skip=/\\>/ end=/>/ contains=thraxBackslashedChar
syn match thraxFuncDef  "\<func\s\+[A-Za-z_][A-Za-z0-9_]*\s*\[" contains=thraxSymbols,thraxIncludedKeyword
syn match thraxIdDef "\(\<export\s\+\)\?[A-Za-z_][A-Za-z0-9_]*\s*=" contains=thraxSymbols,thraxIncludedKeyword
syn match thraxNumber "\<[0-9]\+\>"

hi def link thraxKeyword Keyword
hi def link thraxParseKeyword Keyword
hi def link thraxIncludedKeyword Keyword
hi def link thraxComment Comment
hi def link thraxFuncDef Function
hi def link thraxFstString String
hi def link thraxBackslashedChar Special
hi def link thraxSymbols Operator
hi def link thraxNumber Number
hi def link thraxIdDef Identifier

" Overloading PreProc for non-FST (literal) strings.
hi def link thraxLiteralString PreProc
" Overloading the Type color for our built in functions.
hi def link thraxBuiltinFunctions Type

let b:current_syntax = 'thraxgrm'
