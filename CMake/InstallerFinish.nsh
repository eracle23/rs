; Injected by CPACK_NSIS_INCLUDE_SCRIPT from the superbuild.
; Ensures the Finish page shows a Run checkbox, and defaults to
; launching the per-user association helper.

!verbose push
!verbose 3

; 1) Finish page run target (Run checkbox)
;    Use $INSTDIR so path is resolved at install time.
!ifndef MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN "$INSTDIR\bin\AssocPrompt.exe"
!endif

; 2) Checkbox text & default unchecked
!ifndef MUI_FINISHPAGE_RUN_TEXT
  !define MUI_FINISHPAGE_RUN_TEXT "Set this for .mrml/.mrb as default"
!endif
!define MUI_FINISHPAGE_RUN_NOTCHECKED

!verbose pop

