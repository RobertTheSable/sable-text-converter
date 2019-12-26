; Demonstration on how to include some custom asm for your project

!normalFontWidths ?= $849300
!fixedFontWidth #= !normalFontWidths+200

ORG $838000
LDA #$0001
