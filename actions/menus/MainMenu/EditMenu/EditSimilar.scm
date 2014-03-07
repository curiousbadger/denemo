;;EditSimilar
(define-once EditSimilar::last #f)   
(let ((target #f) (continuations (if (eq? EditSimilar::params 'once) (begin (set! EditSimilar::params #f) #f) #t)))
   (define (get-tag-list get-command) ;;;get-command is d-DirectiveGetNthTag-note
    (let loop ((tags '())(n 0))
        (define tag #f)
        (set! tag (get-command n))
        (if tag
            (begin 
                (set! tags (cons tag tags))
                (loop tags (+ 1 n)))
            tags)))
        
  (define (select-directive type)
    (let ((tags '()))
        (case type
            ((note)
                (set! tags (get-tag-list d-DirectiveGetNthTagStrictNote)))
            ((chord)
                (set! tags (get-tag-list d-DirectiveGetNthTag-chord)))) 
    (if (null? tags)
            #f
        (if (> (length tags) 1)
            (RadioBoxMenuList tags)
            (list-ref tags 0)))))
  
 (define (edit-tag tag default-action)
    (let ((command (with-input-from-string (string-append "d-"  tag) read)))
        (if (defined? command)
            ((eval command (current-module)) "edit")
            (default-action tag))))
  (define (edit)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " Directives"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))

          (case choice
            ((delete) (d-DirectiveDelete-standalone target))
            ((edit) (d-EditObject))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-standalone  target))
            ((#f)  (set! target #f))))

  (define (edit-note)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " on Noteheads"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons (string-append (_ "Edit ") "\""target"\"" (_ " on Notehead"))  'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))
          (case choice
            ((delete) (d-DirectiveDelete-note target))
            ((edit) (edit-tag target d-DirectiveTextEdit-note))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-note target))
            ((#f)  (set! target #f)))
            (d-CursorUp))

  (define (edit-chord)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " on Chords/Notes/Rests"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons  (string-append (_ "Edit ")  "\""target"\"" (_ " Directive") (_ " on this Chord/Note/Rest"))  'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons  (string-append (_ "Edit ")  "\""target"\"" (_ " Directive") (_ " on this Chord/Note/Rest"))  'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))
        (case choice
            ((delete) (d-DirectiveDelete-chord target))
            ((edit) (edit-tag target d-DirectiveTextEdit-chord))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-chord target))
            ((#f)  (set! target #f))))
            
            
  (define (edit-nonprinting)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " Objects"))   'continue)   
              (cons (_ "Change to Printing") 'switch)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Change to Printing")   'switch)
                  (cons (_ "Execute Scheme") 'execute))))              
        (case choice
            ((switch) (if (d-Directive-chord? DenemoWholeMeasureRestTag) (DenemoWholeMeasureRestCommand 'printing)) (d-SetNonprinting #f))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))
  (define (edit-slurstart)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking slur start positions")   'continue)   
              (cons (_ "Delete Slur")   'delete)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Delete Slur")   'delete)
              (cons (_ "Execute Scheme") 'execute))))
          
        (case choice
            ((delete) (d-ToggleBeginSlur)) ;;; make this execute a slur deletion instead
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))
    (define (edit-slurend)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking slur end positions")   'continue)   
              (cons (_ "Delete Slur")   'delete)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Delete Slur")   'delete)
              (cons (_ "Execute Scheme") 'execute))))
        (case choice
            ((delete) (d-ToggleEndSlur));;; make this execute a slur deletion instead
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))
            
            
    (define (edit-tied)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking tied notes")   'continue)   
              (cons (_ "Delete Tie")   'delete)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Delete Tie")   'delete)
              (cons (_ "Execute Scheme") 'execute))))
        (case choice
            ((delete) (d-ToggleTie))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))
             
    (define (edit-tupletstart)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking tuplet start objects")   'continue)   
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Execute Scheme") 'execute))))
        (case choice
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))            
            
    (define (edit-tupletend)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking tuplet end objects")   'continue)   
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Execute Scheme") 'execute))))
        (case choice
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))
            
    (define (edit-timesig)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking time signature change objects")   'continue)   
              (cons (_ "Edit") 'edit)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
             (set! choice (RadioBoxMenu
              (cons (_ "Edit") 'edit)
              (cons (_ "Execute Scheme") 'execute))))
         
          
        (case choice
            ((stop) (set! target #f))
            ((edit) (d-InsertTimeSig)) ;;;better - offer to add beat structure, invisibility etc
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))    
            
    (define (edit-clef)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking clef change objects")   'continue)   
              (cons (_ "Edit") 'edit)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Edit") 'edit)
              (cons (_ "Execute Scheme") 'execute))))
        (case choice
            ((stop) (set! target #f))
            ((edit) (d-InsertClef)) ;;;better - offer  invisibility etc
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))                     
            
    (define (edit-keysig)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking key signature change objects")   'continue)   
              (cons (_ "Sharpen") 'sharpen)
              (cons (_ "Flatten") 'flatten)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Sharpen") 'sharpen)
              (cons (_ "Flatten") 'flatten)
              (cons (_ "Execute Scheme") 'execute))))
        (case choice
            ((stop) (set! target #f))
            ((sharpen) (begin (d-SharpenKeysig)(edit-keysig)))
            ((flatten) (begin (d-FlattenKeysig)(edit-keysig)))
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f)))) 
            
    (define (edit-stemdirection)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
              (cons (_ "Continue seeking stem direction change objects")   'continue)   
              (cons (_ "Edit") 'edit)
              (cons (_ "Execute Scheme") 'execute)
              (cons (_ "Stop") 'stop)))
            (set! choice (RadioBoxMenu
              (cons (_ "Edit") 'edit)
              (cons (_ "Execute Scheme") 'execute))))
         (case choice
            ((stop) (set! target #f))
            ((edit) (d-VoiceSetting)) ;;;offer other options
            ((execute) (d-ExecuteScheme))
            ((#f)  (set! target #f))))              
            
     (define (edit-timesigdir)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " on Time Signature Change Objects"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))
                  
        (case choice
            ((delete) (d-DirectiveDelete-timesig target))
            ((edit) (edit-tag target d-DirectiveTextEdit-timesig))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-timesig target))
            ((#f)  (set! target #f))))
            
              
     (define (edit-clefdir)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " on Clef Change Objects"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))
        (case choice
            ((delete) (d-DirectiveDelete-clef target))
            ((edit) (edit-tag target d-DirectiveTextEdit-clef))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-clef target))
            ((#f)  (set! target #f))))               
            
     (define (edit-keysigdir)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " on Key Change Objects"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))
        (case choice
            ((delete) (d-DirectiveDelete-keysig target))
            ((edit) (edit-tag target d-DirectiveTextEdit-keysig))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-keysig target))
            ((#f)  (set! target #f))))   
              
     (define (edit-voicedir)
        (define choice #f)
        (if continuations 
            (set! choice (RadioBoxMenu
                  (cons (string-append (_ "Continue Seeking ") "\""target"\"" (_ " on Voice Change objects"))   'continue)   
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Stop") 'stop)
                  (cons (_ "Advanced") 'advanced)))
            (set! choice (RadioBoxMenu
                  (cons (_ "Edit") 'edit)
                  (cons (_ "Delete")   'delete)   
                  (cons (_ "Execute Scheme") 'execute)
                  (cons (_ "Advanced") 'advanced))))
        (case choice
            ((delete) (d-DirectiveDelete-stemdirective target))
            ((edit) (edit-tag target d-DirectiveTextEdit-stemdirective))
            ((stop) (set! target #f))
            ((execute) (d-ExecuteScheme))
            ((advanced) (d-DirectiveTextEdit-stendirective target))
            ((#f)  (set! target #f))))   
                          
                                    
             
;;; the actual procedure
  (let ((type #f))
  
    (if (eq? EditSimilar::params 'continue)
        (if EditSimilar::last
            (begin
                (set! EditSimilar::params #f)
                (set! type (car EditSimilar::last))
                (set! target (cdr EditSimilar::last)))
            (begin
                (set! EditSimilar::params #f)
                (set! continuations 'menu)
                (d-WarningDialog (_ "Cannot resume - no previous search.\nOffering a menu of all possible searches instead.")))))
     (disp "0. type is " type " and target " target "\n\n")           

    (if EditSimilar::params 
        (case (cdr EditSimilar::params)
            ((standalone)
                (set! type 'standalone)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-Directive-standalone? target))))
  
            ((note)
                (set! type 'note)
                (set! target (car EditSimilar::params))
                (FindNextNoteAllColumns (lambda () (d-DirectiveGetForTagStrictNote target))))
                
             ((chord) 
                (set! type 'chord)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-Directive-chord? target))))
             ((nonprinting) 
                (set! type 'nonprinting)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-GetNonprinting target))))
                
             ((slurstart) 
                (set! type 'slurstart)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-IsSlurStart))))
                
             ((slurend) 
                (set! type 'slurend)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-IsSlurEnd))))
                
             ((tied) 
                (set! type 'tied)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-IsTied))))
                    
              ((tupletstart) 
                (set! type 'tupletstart)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (TupletOpen?))))               
              ((tupletend) 
                (set! type 'tupletend)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (TupletClose?))))
                
              ((timesig) 
                (set! type 'timesig)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (Timesignature?))))                
              ((keysig) 
                (set! type 'keysig)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (Keysignature?))))                
              ((stemdirection) 
                (set! type 'stemdirection)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (StemDirective?))))                
             ((clef) 
                (set! type 'clef)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (Clef?))))                       
                
              ((voicedir) 
                (set! type 'voicedir)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-Directive-stemdirective? target))))                
              ((keysigdir) 
                (set! type 'keysigdir)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-Directive-keysig? target)))) 
              ((timesigdir) 
                (set! type 'timesigdir)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-Directive-timesig? target)))) 
               ((clefdir) 
                (set! type 'clefdir)
                (set! target (car EditSimilar::params))
                (FindNextObjectAllColumns (lambda () (d-Directive-clef? target)))) 
                        
                               
                                                  
             (else
                (disp "Not handling " EditSimilar::params " yet.")
                (set! EditSimilar::params #f))))
     (disp "1. type is " type " and target " target "\n\n")           
                 
    (if (not type)
        (begin
            (set! target (d-DirectiveGetTag-standalone))
            (if target 
                (set! type 'standalone)
                (begin
                    (set! target (select-directive 'note))
                    (if target
                        (set! type 'note)
                        (begin
                            (set! target (select-directive 'chord))
                            (if target
                                (set! type 'chord)
                                (if (d-IsSlurStart)
                                    (set! type 'slurstart)
                                    (if (d-IsSlurEnd)
                                        (set! type 'slurend)
                                        (if (d-IsTied)
                                            (set! type 'tied)
                                            (if (d-GetNonprinting)
                                                (set! type 'nonprinting)
                                                (if (TupletOpen?)
                                                    (set! type 'tupletstart)
                                                    (if (TupletClose?)
                                                        (set! type 'tupletend)
                                                        (if (Timesignature?)
                                                            (set! type 'timesig)
                                                            (if (Clef?)
                                                                (set! type 'clef)
                                                                (if (Keysignature?)
                                                                    (set! type 'keysig)
                                                                    (if (StemDirective?)
                                                                        (set! type 'stemdirection)
                                                                        (if (d-Directive-timesig? target)
                                                                            (set! type 'timesigdir)
                                                                            (if (d-Directive-keysig? target)
                                                                                (set! type 'keysigdir)
                                                                                (if (d-Directive-stemdirective? target)
                                                                                    (set! type 'voicedir)
                                                                                    (if (d-Directive-clef? target)
                                                                                        (set! type 'clefdir))))))))))))))))))))))
                                                            
                                                            
                                                            
       (disp "2. type is " type " and target " target "\n\n")           
                                                          
                                                            
    (set! EditSimilar::last (cons type target))
                            
    (case type
             ((standalone)
                          (edit)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-Directive-standalone? target))))
                              (edit))))
            ((note)
                          (edit-note)
                          (if continuations (while (and target (FindNextNoteAllColumns (lambda () (d-DirectiveGetForTagStrictNote target))))
                              (edit-note))))
            ((chord)
                          (edit-chord)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-Directive-chord? target))))
                              (edit-chord))))
                              
            ((nonprinting)
                          (edit-nonprinting)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-GetNonprinting))))
                              (edit-nonprinting))))                 
            ((slurstart)
                          (edit-slurstart)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-IsSlurStart))))
                              (edit-slurstart)))) 
                              
            ((slurend)
                          (edit-slurend)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-IsSlurEnd))))
                              (edit-slurend))))

            ((tied)
                          (edit-tied)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-IsTied))))
                              (edit-tied))))
                              
            ((tupletstart)
                          (edit-tupletstart)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (TupletOpen?))))
                              (edit-tupletstart)))) 
            ((tupletend)
                          (edit-tupletend)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (TupletClose?))))
                              (edit-tupletend))))                                                             
 
 
            ((timesig)
                          (edit-timesig)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (Timesignature?))))
                              (edit-timesig))))        
            ((clef)
                          (edit-clef)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (Clef?))))
                              (edit-clef))))                                                             
            ((keysig)
                          (edit-keysig)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (Keysignature?))))
                              (edit-keysig))))                                                             
            ((stemdirection)
                          (edit-stemdirection)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (StemDirective?))))
                              (edit-stemdirection))))                                                                                                                       
            ((timesigdir)
                          (edit-timesigdir)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-Directive-timesig? target))))
                              (edit-timesigdir))))            
            ((keysigdir)
                          (edit-keysigdir)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-Directive-keysig? target))))
                              (edit-keysigdir))))            
            ((voicedir)
                          (edit-voicedir)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-Directive-stemdirective? target))))
                              (edit-voicedir))))            
                        
            ((clefdir)
                          (edit-clefdir)
                          (if continuations (while (and target (FindNextObjectAllColumns (lambda () (d-Directive-clef? target))))
                              (edit-clefdir))))            
                              
                                                       
            (else
                (if continuations
                    (if (eq? continuations 'menu)
                        (d-ChooseSeekEditDirectives)
                        (d-InfoDialog (_ "Attributes and Directives attached to noteheads, chords (including notes and rests) and standalone objects are supported - position the cursor on a notehead for directives on that notehead or off the noteheads for directives on a chord/note/rest, or on any other sort of object in the music. \nAlternatively, use \"Choose, Seek and Edit\" command to select from a list of types of directives in the movement to seek for.")))
                    (d-EditObject))))
    (disp "3. type is " type " and target " target "\n\n")           
               
    (if continuations
        (let ((choice #f))
            (set! choice (RadioBoxMenu
                (cons (_ "Wrap to beginning") 'wrap)
                (if (LastMovement?)
                    (if (FirstMovement?)
                        (cons (_ "Stop") 'stop)
                        (cons (_ "Wrap to first movement") 'first))
                    (cons (_ "Wrap to next movement") 'wrapmovement))))

            (case choice
                ((wrap) 
                    (d-MoveToBeginning)
                    (d-EditSimilar (cons target type)))
                 ((first) 
                    (while (d-PreviousMovement))
                    (d-MoveToBeginning)
                    (d-EditSimilar (cons target type)))
                ((wrapmovement) 
                        (if (d-NextMovement)
                                (begin
                                    (d-MoveToBeginning)
                                    (d-EditSimilar (cons target type)))))
                (else (TimedNotice (_ "Finished"))))))))
