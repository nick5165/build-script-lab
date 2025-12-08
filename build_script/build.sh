#!/bin/sh

cleanup() {
    exit_code=$?
    if [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
    fi
    exit $exit_code
}

trap cleanup INT TERM EXIT

if [ $# -ne 1 ]; then
    echo "Ошибка: Не указан исходный файл." >&2
    echo "Использование: $0 <исходный_файл>" >&2
    exit 1
fi

SOURCE_FILE="$1"

if [ ! -f "$SOURCE_FILE" ] || [ ! -r "$SOURCE_FILE" ]; then
    echo "Ошибка: Файл '$SOURCE_FILE' не существует или недоступен для чтения." >&2
    exit 2
fi
OUTPUT_NAME=$(grep 'Output:' "$SOURCE_FILE" | head -1 | sed 's/.*Output:[[:space:]]*//' | sed 's/[[:space:]]*$//')

if [ -z "$OUTPUT_NAME" ]; then
    echo "Ошибка: В файле '$SOURCE_FILE' не найден комментарий с указанием выходного файла." >&2
    echo "Пример комментария: 'Output: myprogram' или '% Output: document.pdf'" >&2
    exit 3
fi

TEMP_DIR=$(mktemp -d)
if [ $? -ne 0 ]; then
    echo "Ошибка: Не удалось создать временный каталог." >&2
    exit 4
fi

echo "Сборка в временном каталоге: $TEMP_DIR"
echo "Целевой файл: $OUTPUT_NAME"
case "$SOURCE_FILE" in
    *.c)
        cp "$SOURCE_FILE" "$TEMP_DIR/"
        if ! gcc "$TEMP_DIR/$(basename "$SOURCE_FILE")" -o "$TEMP_DIR/$OUTPUT_NAME"; then
            echo "Ошибка: Компиляция C программы не удалась." >&2
            exit 5
        fi
        ;;
    *.cpp|*.cxx|*.cc)
        cp "$SOURCE_FILE" "$TEMP_DIR/"
        if ! g++ "$TEMP_DIR/$(basename "$SOURCE_FILE")" -o "$TEMP_DIR/$OUTPUT_NAME"; then
            echo "Ошибка: Компиляция C++ программы не удалась." >&2
            exit 6
        fi
        ;;
    *.tex)
        cp "$SOURCE_FILE" "$TEMP_DIR/"
        cd "$TEMP_DIR" || exit 7
        if ! pdflatex -interaction=nonstopmode "$(basename "$SOURCE_FILE")" > /dev/null; then
            echo "Ошибка: Первый проход LaTeX компиляции не удался." >&2
            exit 8
        fi
        if ! pdflatex -interaction=nonstopmode "$(basename "$SOURCE_FILE")" > /dev/null; then
            echo "Ошибка: Второй проход LaTeX компиляции не удался." >&2
            exit 9
        fi
        cd - > /dev/null
        ;;
    *)
        echo "Ошибка: Неподдерживаемый тип файла. Поддерживаются .c, .cpp, .cxx, .cc, .tex" >&2
        exit 10
        ;;
esac
if ! cp "$TEMP_DIR/$OUTPUT_NAME" "./"; then
    echo "Ошибка: Не удалось скопировать '$OUTPUT_NAME' в текущий каталог." >&2
    exit 11
fi

echo "Сборка успешно завершена! Файл '$OUTPUT_NAME' создан."
