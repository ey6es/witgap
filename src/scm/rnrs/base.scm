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

(define-syntax when
    (syntax-rules ()
        ((when test firstExpr restExpr ...)
         (if test (begin firstExpr restExpr ...)))
    )
)

(define-syntax unless
    (syntax-rules ()
        ((unless test firstExpr restExpr ...)
         (if (not test) (begin firstExpr restExpr ...)))
    )
)

(define-syntax do
    (syntax-rules ()
        ((do ((var init step) ...)
            (test expr ...)
            command ...)
         (let loop ((var init) ...)
            (if test (begin #f expr ...) (begin
                command ...
                (loop step ...)))))
    )
)

(define (values . things)
    (call-with-current-continuation
        (lambda (cont) (apply cont things))
    )
)

(define (caar ll) (car (car ll)))
(define (cdar ll) (cdr (car ll)))
(define (cadr ll) (car (cdr ll)))
(define (cddr ll) (cdr (cdr ll)))

(define (cars lists)
    (if (null? lists) '()
        (cons (car (car lists)) (cars (cdr lists))))
)

(define (cdrs lists)
    (if (null? lists) '()
        (cons (cdr (car lists)) (cdrs (cdr lists))))
)

(define (map proc firstList . restLists)
    (if (null? firstList) '()
        (cons (apply proc (cons (car firstList) (cars restLists)))
            (apply map (cons proc (cons (cdr firstList) (cdrs restLists))))))
)

(define (for-each proc firstList . restLists)
    (if (not (null? firstList)) (begin
        (apply proc (cons (car firstList) (cars restLists)))
        (apply for-each (cons proc (cons (cdr firstList) (cdrs restLists))))
    ))
)

(define (vector-refs vectors k)
    (if (null? vectors) '()
        (cons (vector-ref (car vectors) k) (vector-refs (cdr vectors) k))
    )
)

(define (vector-map proc firstVector . restVectors)
    (define len (vector-length firstVector))
    (define vectors (cons firstVector restVectors))
    (define mapped (make-vector len))
    (do ((idx 0 (+ idx 1))) ((= idx len) mapped)
        (vector-set! mapped idx
            (apply proc (vector-refs vectors idx))))
)

(define (vector-for-each proc firstVector . restVectors)
    (define len (vector-length firstVector))
    (define vectors (cons firstVector restVectors))
    (do ((idx 0 (+ idx 1))) ((= idx len))
        (apply proc (vector-refs vectors idx)))
)

(define (string-refs strings k)
    (if (null? strings) '()
        (cons (string-ref (car strings) k) (string-refs (cdr strings) k))
    )
)

(define (string-for-each proc firstString . restStrings)
    (define len (string-length firstString))
    (define strings (cons firstString restStrings))
    (do ((idx 0 (+ idx 1))) ((= idx len))
        (apply proc (string-refs strings idx)))
)
