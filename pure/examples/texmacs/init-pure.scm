;; Here are a few sample Pure sessions for TeXmacs. You might want to add
;; other options as needed. NOTE: To have TeXmacs find these, drop this file
;; into your ~/.TeXmacs/plugins/pure/progs folder (create the path if needed).

(plugin-configure pure
  (:require (url-exists-in-path? "pure"))
  (:launch "pure -i --texmacs")
  (:session "Pure"))

;; plain signon (less fancy)
(plugin-configure pure-plain
  (:require (url-exists-in-path? "pure"))
  (:launch "pure -i --plain --texmacs")
  (:session "Pure-plain"))

;; without signon ("quiet")
(plugin-configure pure-quiet
  (:require (url-exists-in-path? "pure"))
  (:launch "pure -i -q --texmacs")
  (:session "Pure-quiet"))

;; debugging session
(plugin-configure pure-debug
  (:require (url-exists-in-path? "pure"))
  (:launch "pure -i -g --texmacs")
  (:session "Pure-debug"))
