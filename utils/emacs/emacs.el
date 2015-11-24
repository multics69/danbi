;; DANBI coding style guidelines in emacs
;; Modified:   2012-05-20

;; cscope
(require 'xcscope) 

;; Auto insert C/C++ header
(require 'autoinsert)
(auto-insert-mode)
(setq auto-insert-query nil)
(define-auto-insert "\.c" "template.c")
(define-auto-insert "\.cpp" "template.c")
(define-auto-insert "\.h" "template.c")

(setq org-auto-insert-directory auto-insert-directory)
(setq danbi-auto-insert-directory "~/workspace/danbi.src/utils/emacs")


;; Open *.cu and *.cl in C-mode
(setq auto-mode-alist (cons '("\.cu$" . c-mode) auto-mode-alist))
(setq auto-mode-alist (cons '("\.cl$" . c-mode) auto-mode-alist))

;; Open *.dnb in C++-mode
(setq auto-mode-alist (cons '("\.dnb$" . c++-mode) auto-mode-alist))

;; Open *.str in C-mode
(setq auto-mode-alist (cons '("\.str$" . c-mode) auto-mode-alist))

;; Max 80 cols per line, indent by two spaces, no tabs.
;; Apparently, this does not affect tabs in Makefiles.
(custom-set-variables
  '(fill-column 80)
  '(c++-indent-level 2)
  '(c-basic-offset 2))

;; Alternative to setting the global style.  Only files with "llvm" in
;; their names will automatically set to the llvm.org coding style.
(c-add-style "danbi.org"
             '((fill-column . 80)
	       (c++-indent-level . 2)
	       (c-basic-offset . 2)
	       (indent-tabs-mode . nil)
               (c-offsets-alist . ((innamespace 0)))))

(add-hook 'c-mode-hook
	  (function
	   (lambda nil 
	     (if (string-match "danbi" buffer-file-name)
		 (progn
		   (c-set-style "danbi.org")
                   (setq auto-insert-directory danbi-auto-insert-directory)
		   )
                 (setq auto-insert-directory org-auto-insert-directory)
	       ))))

(add-hook 'c++-mode-hook
	  (function
	   (lambda nil 
	     (if (string-match "danbi" buffer-file-name)
		 (progn
		   (c-set-style "danbi.org")
                   (setq auto-insert-directory danbi-auto-insert-directory)
		   )
                 (setq auto-insert-directory org-auto-insert-directory)
	       ))))

