;;; Figured Bass filter


(define-once ToggleFiguredBassMode::Active #f)
    
(if (not ToggleFiguredBassMode::Active)
    (let 
	((Figures 0) (AddFigure 0) (GetFigures 0) (loop 0) (PedalDown #f) (Notes '()))
	 (begin
	  (d-SetBackground #xB0E0B0)
	   (set! AddFigure (lambda (note bassnote)
			     (let ((fig (d-BassFigure bassnote note)))
			      (if (car fig);extreme interval - warn
				(d-PlayMidiNote note 255 15 350))
			      (set! Notes (cons note Notes))
			     ; (disp Notes "\n")
			      (set! Figures (cons  (cdr fig) Figures)))))
			      
	   (set! GetFigures (lambda ()
			      (begin
				(string-append "figures=" (string-join (reverse Figures))))))
	   (set! loop  (lambda (bassnote)
			 (let* ( 
				(midi (d-GetMidi))
				(velocity (bit-extract midi 16 24))
				(note (bit-extract midi 8 16))
				(command (bit-extract midi 0 8)))
			   ;; body of the let*
			       (begin
				 (if (boolean? (d-GetNoteName))
				     (d-NextNote))
				 (if (or (= command #x90) (= command #x80))
				     (begin
				       (if (and (= command #x90)(= bassnote 0))
					   (begin 
					     (set! bassnote note)
					     (set! Figures '())
					     (set! Notes '())
					     (if (or (d-GetNonprinting) (= bassnote (d-GetNoteAsMidi)))
					      (begin;; note ok
					       (PlayNote (number->string note) 500 "127")
					       (if PedalDown
						  (begin 
						  (d-PlayMidiNote 60 255 9 100)
						  (set! Figures (cons "~"  Figures))
						  (d-EditFiguredBass (GetFigures))
						  (set! Figures '())
						  (set! Notes '())
						  (set! bassnote 0)						  
  						   (if (boolean? (d-GetNoteName))
		 					  (d-NextNote)))
					       ))				   
				    	      (begin                    ;; note wrong
				    	  	 (set! bassnote 0)
						(d-PlayMidiNote 35 255 9 100))
				    	      )  
					     );;; end of  noteon while we have no bass note
			     ;;; not attempted bass note on 
					   (begin
					     (if (and (= command #x80)(= note bassnote))
						 (begin ;;;bass note off get the figures (moves on) and loop
						   (if (not (d-SpellCheckMidiChord (cons bassnote Notes)))
						   	(d-PlayMidiNote 30 255 9 100))
						   (set! bassnote 0)
						   (d-EditFiguredBass (GetFigures))
  						   (if (boolean? (d-GetNoteName))
		 					  (d-NextNote))
									);;; end of received bassnote off
				 ;;; not bass note off
						 (begin
						   (if  (= command #x90)
							(begin
							  (AddFigure note bassnote)
							  (PlayNote (number->string note) 500 "127")
							  )))))));; end of if noteon or noteoff				  
				(begin ;not noteon/off
				 (if (and (= command #xB0) (= note #x40) (= velocity #x7F))	   
				     (begin;; Pedal Down
					(set! PedalDown #t)
				       	(display "Pedal down")
					(if (and (> bassnote 0) (null? Figures))
						(begin 
						;;; here check if the previous note has a figure, if not put a 0 figure to be able to continue it
						  (if (d-PrevNote)
						  	(let ((fig (d-EditFiguredBass "query")))
						  		(if (not fig)
						  		     (d-EditFiguredBass "figures=0")
						  		     (d-NextNote))))					
						  (d-PlayMidiNote 49 255 9 100)
						  (set! Figures (cons "~"  Figures))
						  (d-EditFiguredBass (GetFigures))
						  (set! Figures '())
						  (set! Notes '())
						  (set! bassnote 0))
						(begin
						 (d-PlayMidiNote 71 255 9 100)
						  (set! Figures (cons " | "  Figures))))))
				(if (and (= command #xB0) (= note #x40) (= velocity #x0))	      
				      (set! PedalDown #f))
				(if (and (= command #xB0)   (= note 1))
				     	 (let ((thestep  (round(/ (- velocity 64) 16))))
					   (PlayNote  
					    (number->string  (+ 60 (* 4 thestep) ))
					    100)	     
					   (eval-string (string-append "(d-SetEnharmonicPosition " (number->string  thestep) ")"))))))
					   
			    (if (= midi 0)
				 	(set! ToggleFiguredBassMode::Active #f))
				 
			    (if ToggleFiguredBassMode::Active 
				     (loop bassnote))
				 ));; end of let, the body of the loop proc
			   )) ;;; loop definition
			   
	   (d-InputFilterNames "Figured Bass MIDI Filter")
    	   (set! ToggleFiguredBassMode::Active #t)
	   (d-SetMidiCapture #t)	   
	   (loop 0)
	   
	  ;; (d-SetMidiCapture #f)
					   ));;; end of not active block
						    );;; end of if active/not active if
	   (d-InputFilterNames "No active MIDI Filter")
    	   (set! ToggleFiguredBassMode::Active #f)
    	   (d-PutMidi 0);;; to swallow up the last d-GetMidi??
    	   (d-SetMidiCapture #f)	
    	   (d-SetBackground #xFFFFFF)
	   
