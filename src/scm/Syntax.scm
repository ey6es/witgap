;
; $Id$

(define-syntax let
    (syntax-rules ()
        ((let ((var init) ...) expr ...)
         ((lambda (var ...) expr ...) init ...))
    )
)
