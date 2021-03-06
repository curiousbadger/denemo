;;;DeleteFromCursorToEnd
(let ((choice (RadioBoxMenu (cons (_ "Cancel") #f) (cons (_ "All Staffs - Delete all measures from cursor?") 'all) (cons (_ "This Staff/Voice only?") 'this))))
                 (case choice
                    ((all)
                    	(d-SetSaved #f)
                    	(d-EvenOutStaffLengths)
                        (while (d-MoveToMeasureRight)
                            (d-MoveToMeasureLeft)
                            (d-DeleteMeasureAllStaffs))
                          (d-DeleteMeasureAllStaffs))
                   ((this)
                        (d-SetSaved #f)
                        (while (d-MoveToMeasureRight)
                            (d-MoveToMeasureLeft)
                            (d-DeleteMeasure))
                          (d-DeleteMeasure))
                    (else        
                        (d-InfoDialog (_ "Cancelled")))))
