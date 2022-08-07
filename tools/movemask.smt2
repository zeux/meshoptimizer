; $ z3 movemask.smt2
; sat
; (model
;   (define-fun magic () (_ BitVec 64)
;     #x000103070f1f3f80)
; )

(set-logic BV)

(declare-const magic (_ BitVec 64))

(define-fun same ((v (_ BitVec 8))) Bool
 (or (= v #x00) (= v #xFF)))

(assert
  (forall ((x (_ BitVec 64)))
    (let ((y (bvmul x magic)))
      (or
        (not
          (and
           (same ((_ extract 63 56) x))
           (same ((_ extract 55 48) x))
           (same ((_ extract 47 40) x))
           (same ((_ extract 39 32) x))
           (same ((_ extract 31 24) x))
           (same ((_ extract 23 16) x))
           (same ((_ extract 15 8) x))
           (same ((_ extract 7 0) x))
           )
        )
        (and
          (= ((_ extract 63 63) x) ((_ extract 63 63) y))
          (= ((_ extract 55 55) x) ((_ extract 62 62) y))
          (= ((_ extract 47 47) x) ((_ extract 61 61) y))
          (= ((_ extract 39 39) x) ((_ extract 60 60) y))
          (= ((_ extract 31 31) x) ((_ extract 59 59) y))
          (= ((_ extract 23 23) x) ((_ extract 58 58) y))
          (= ((_ extract 15 15) x) ((_ extract 57 57) y))
          (= ((_ extract  7  7) x) ((_ extract 56 56) y))
        )
      )
    )
  )
)

(check-sat)
(get-model)