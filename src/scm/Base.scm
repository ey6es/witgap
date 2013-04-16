;
; $Id: Syntax.scm 194 2012-10-01 02:19:58Z andrzej $

(define (values . things)
  (call-with-current-continuation
    (lambda (cont) (apply cont things))))

(define (call-with-values producer consumer)
    (consumer (producer)))
