;;;PageBreaksBeforeMovements
(let ((params PageBreaksBeforeMovements::params))
	(if (not params)
		(set! params (d-Directive-movementcontrol? "MovementPageBreak")))
	(d-PushPosition)
	(while (d-PreviousMovement))
	(while  (d-NextMovement)
		(d-MovementPageBreak #t)
		(if (not  (eq? params #t))
			(d-MovementPageBreak #f)))
	(d-PopPosition))