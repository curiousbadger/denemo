;;;AddDuplicateMeasure
(d-PushClipboard)
(d-SelectMeasure)
(d-Copy)
(if (d-MoveToMeasureRight)
	(if (None?)
		#t ; Empty measure
		(d-AddMeasure)) ; create empty measure
	(begin ; End of staff
		(d-AppendMeasureAllStaffs)
		(d-MoveToMeasureRight)))
(d-Paste)
(d-PopClipboard)