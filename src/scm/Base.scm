;
; $Id: Syntax.scm 194 2012-10-01 02:19:58Z andrzej $

(define (values . things)
    (call-with-current-continuation
        (lambda (cont) (apply cont things))
    )
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
    (for (idx 0 len)
        (vector-set! mapped idx
            (apply proc (vector-refs vectors idx))))
    mapped
)

(define (vector-for-each proc firstVector . restVectors)
    (define len (vector-length firstVector))
    (define vectors (cons firstVector restVectors))
    (for (idx 0 len)
        (apply proc (vector-refs vectors idx)))
)
