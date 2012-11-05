
LATEX=pdflatex
NAME=manual

QUIET_LATEX    = @echo ' ' LATEX $@;                 
	
all: $(NAME)

%.pdf: %.tex
	$(QUIET_LATEX)$(LATEX) $*.tex $< >/dev/null
	$(QUIET_LATEX)$(LATEX) $*.tex $< >/dev/null

$(NAME): $(NAME).pdf

clean:
	rm -rf $(NAME).aux $(NAME).log $(NAME).toc $(NAME).pdf