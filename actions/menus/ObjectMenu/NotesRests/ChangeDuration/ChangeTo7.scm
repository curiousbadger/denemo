;;;ChangeTo7
(let ((shift #f))
    (if (Appending?)
        (begin
            (set! shift #t)
            (d-MoveCursorLeft)))
    (if (Music?)
        (d-Change7))
    (if shift
        (d-MoveCursorRight)))
