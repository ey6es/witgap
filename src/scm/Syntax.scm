;
; $Id$

(define-syntax let
    (syntax-rules ()
        ((let ((var init) ...) expr ...)
         ((lambda (var ...) expr ...) init ...))
        
        ((let name ((var init) ...) expr ...)
         (letrec ((name (lambda (var ...) expr ...)))
          (name init ...)))
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

(define-syntax letrec
    (syntax-rules ()
        ((letrec ((var init) ...) expr ...)
         ((lambda () (define var init) ... expr ...)))
    )
)

(define-syntax letrec*
    (syntax-rules ()
        ((letrec* ((var init) ...) expr ...)
         ((lambda () (define var init) ... expr ...)))
    )
)

(define-syntax let-values
    (syntax-rules ()
        ((let-values (((formal ...) (_ init ...)) ...) expr ...)
         ((lambda (formal ... ...) expr ...) init ... ...))
    )
)

(define-syntax let*-values
    (syntax-rules ()
        ((let*-values () expr ...)
         ((lambda () expr ...)))
         
        ((let*-values (((formal ...) (_ init ...))) expr ...)
         ((lambda (formal ...) expr ...) init ...))

        ((let*-values (((f0 ...) (_ i0 ...)) ((f1 ...) (_ i1 ...)) ...) expr ...)
         ((lambda (f0 ...) (let*-values (((f1 ...) (values i1 ...)) ...) expr ...)) i0 ...))
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

(define-syntax cond
    (syntax-rules (else =>)
        ((cond (test expr))
         (if test expr))
         
        ((cond (test => expr))
         (if test expr))
         
        ((cond (test expr) (else elsexp))
         (if test expr elsexp))
         
        ((cond (test => expr) (else elsexp))
         (if test expr elsexp))
         
        ((cond (test expr) rest ...)
         (if test expr (cond rest ...)))
         
        ((cond (test => expr) rest ...)
         (if test expr (cond rest ...)))
    )
)


