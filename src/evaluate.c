//To ensure pointers can be passed through longjmp
char longjmp_hack[sizeof(int) - sizeof(void *)];
#include "signal.h"
#include "compile.c"
#include "evaluate_errors.c"

volatile bool input_finished = false;

void int_handler() {
	input_finished = true;
}

#define INPUT_BUFFER_SIZE 1024

char *open_prompt(jmp_buf *handler) {
	printf("- ");
	char str[INPUT_BUFFER_SIZE];
	char *outfn = cprintf("%s", "./XXXXXX.l2");
	FILE *l2file = fdopen(mkstemps(outfn, 3), "w+");
	if(!l2file) return NULL;
	while(fgets(str, INPUT_BUFFER_SIZE, stdin)) {
		if(input_finished) {
			longjmp(*handler, (int) make_no());
		} else if(strlen(str) + 1 == INPUT_BUFFER_SIZE) {
			fclose(l2file);
			exit(0);
		}
		fputs(str, l2file);
		printf("+ ");
	}
	fflush(l2file);
	return outfn;
}

int main(int argc, char *argv[]) {
	list shared_library_names = nil(), shared_library_handles = nil();
	volatile int processing_from, processing_to, i;
	
	//Initialize the error handler
	jmp_buf handler;
	union evaluate_error *err;
	if(err = (union evaluate_error *) setjmp(handler)) {
		if(err->no.type != NO) {
			printf("Error found between %s and %s inclusive: ", argv[processing_from], argv[processing_to - 1]);
		}
		print_annotated_syntax_tree_annotator = &empty_annotator;
		switch(err->no.type) {
			case PARAM_COUNT_MISMATCH: {
				printf("The number of arguments in ");
				print_annotated_syntax_tree(err->param_count_mismatch.src_expression);
				printf(" does not match the number of parameters in ");
				print_annotated_syntax_tree(err->param_count_mismatch.dest_expression);
				printf(".\n");
				break;
			} case SPECIAL_FORM: {
				if(err->special_form.subexpression_list) {
					printf("The subexpression ");
					print_expr_list(err->special_form.subexpression_list);
					printf(" does not satisfy the requirements of the expression ");
					print_expr_list(err->special_form.expression_list);
					printf(".\n");
				} else {
					printf("The expression ");
					print_expr_list(err->special_form.expression_list);
					printf(" has an incorrect number of subexpressions.\n");
				}
				break;
			} case UNEXPECTED_CHARACTER: {
				printf("Unexpectedly read %c at %ld.\n", err->unexpected_character.character, err->unexpected_character.position);
				break;
			} case MULTIPLE_DEFINITION: {
				printf("The definition of %s erroneously occured multiple times.\n", err->multiple_definition.reference_value);
				break;
			} case ENVIRONMENT: {
				printf("The following occured when trying to use an environment: %s\n", err->environment.error_string);
				break;
			} case MISSING_FILE: {
				printf("File is missing.\n");
				return MISSING_FILE;
			} case ARGUMENTS: {
				printf("Bad command line arguments.\n");
				printf("Usage: l2compile objects.o ... (- inputs.l2 ...) ... - inputs.l2 ...\n\n"
					"Uses objects.o ... as libraries for remaining stages of the compilation and, if the final output is\n"
					"not an object file, embeds them into the final output. Concatenates the first group inputs.l2 ...,\n"
					"compiles the concatenation, and uses the executable as an environment for the remaining stages of\n"
					"compilation. Does the same process repeatedly until the last group is reached. Finally, concatenates\n"
					"last group, compiles concatenation into either a position independent or dependent object, shared\n"
					"library, or program called output as specified by the flags.\n");
				return ARGUMENTS;
			}
		}
		if(processing_from == argc && err->no.type != NO) {
			i = argc - 1;
			goto prompt;
		} else {
			void *handle;
			{foreach(handle, shared_library_handles) {
				dlclose(handle);
			}}
			char *filename;
			foreach(filename, shared_library_names) {
				remove(filename);
			}
			return err->no.type;
		}
	}
	
	processing_from = 1;
	processing_to = argc;
	
	if(argc < 2) {
		longjmp(handler, (int) make_arguments());
	}
	for(i = 1; i < argc && strcmp(argv[i], "-"); i++);
	if(i == argc) {
		longjmp(handler, (int) make_arguments());
	}
	char *init_library = nil_library();
	for(i = 1; i < argc && strcmp(argv[i], "-"); i++) {
		init_library = sequence(init_library, argv[i]);
	}
	dlopen(dynamic(init_library), RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND);
	
	signal(SIGINT, int_handler);
	prompt: while(i < argc) {
		int j;
		list expressions = nil();
		list expansion_lists = nil();
		char *source = nil_source();
		
		for(j = ++i; (j == argc && i == argc) || (i < argc && strcmp(argv[i], "-")); i++) {
			processing_from = i;
			processing_to = i + 1;
			source = concatenate(source, (j == argc) ? open_prompt(&handler) : argv[i]);
		}
		processing_from = j;
		processing_to = i;
		if(j == argc) i -= 2;
		
		char *sofn = dynamic(library(source, &handler));
		void *handle = dlopen(sofn, RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND);
		if(!handle) {
			longjmp(handler, (int) make_environment(cprintf("%s", dlerror())));
		} else if(processing_from == argc) {
			printf("\n");
		}
		prepend(sofn, &shared_library_names);
		prepend(handle, &shared_library_handles);
	}
	printf("\n");
	longjmp(handler, (int) make_no());
}
