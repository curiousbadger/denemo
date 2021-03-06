;;;RehearsalMarkNonPrinting 
 ;;;Non Printing Bookmark 
;;;by Nils Gey RTS Modified to avoid poluting the global namespace with the variable user-input

(let ((user-input "XXX"))
(set! user-input (d-GetUserInput "Named Bookmark" "Give a name" "X"))

 (if user-input   ;in case the user pressed Escape do nothing
 (begin
  (d-Directive-standalone "RehearsalMark")
  (d-DirectivePut-standalone-display "RehearsalMark" user-input)
  (d-DirectivePut-standalone-ty  "RehearsalMark" -40)
  (d-DirectivePut-standalone-tx  "RehearsalMark" -10)
  ))
(d-RefreshDisplay))

