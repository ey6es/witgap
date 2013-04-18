;
; $Id: Syntax.scm 194 2012-10-01 02:19:58Z andrzej $

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
