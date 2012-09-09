;
; $Id$

(define-syntax let
    (syntax-rules ()
        ((let ((var init) ...) expr ...)
         ((lambda (var ...) expr ...) init ...))
    )
)

(define-syntax let*
    (syntax-rules ()
        ((let* () expr ...)
         ((lambda () expr ...)))
        ((let* ((var init)) expr ...)
         ((lambda (var) expr ...) init))
        ((let* ((v0 i0) (v1 i1) (vn in) ...) expr ...)
         ((lambda (v0) (let* ((v1 i1) (vn in) ...) expr ...)) i0))
    )
)

(define-syntax and
    (syntax-rules ()
        ((and) #t)
        ((and var) var)
        ((and var first rest ...) (if var (and first rest ...) #f))
    )
)

(define-syntax or
    (syntax-rules ()
        ((or) #f)
        ((or var) var)
        ((or var first rest ...) (if var #t (or first rest ...)))
    )
)
