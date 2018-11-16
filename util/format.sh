#!/usr/bin/env bash
IFS=$'\n'
set -f

all=0
if [ "$1" == "-a" ]; then
    shift
    all="1"
fi

[ -n "$1" ] && {
  echo "$0: Too many arguments" >&2
  exit 1
}

have_changes=0       # We have uncommitted changes
authored_previous=0  # We authored the most recent commit on this branch
git diff --exit-code --quiet || have_changes=1
git log --exit-code "HEAD~1..HEAD" --author "\\<$USER@" > /dev/null ||
    authored_previous=1

files=()

if (( all )); then
  echo "Formatting all files..."
  files=(
    $(find -name \*.h -print -o -name \*.cpp -print \
      | grep -v third-party)
  )
elif (( have_changes )); then
  echo "Formatting only modified files..."
  files=(
      $(git diff --name-only -- '*.cpp' '*.h' \
        | grep -v third-party)
  )

elif (( authored_previous )); then
  echo "There are no modified files, but you authored the last commit:"
  git log --format="%h %s" "HEAD~1..HEAD"
  echo
  read -r -p "Format files in that commit? [y/N]" reply
  if [[ $reply == [Yy] ]]; then
    files=(
      $(git diff --name-only "HEAD~1..HEAD" -- '*.cpp' '*.h' \
        | grep -v third-party)
    )
  else
    exit 1
  fi

else
  echo -n "You have no modified files and you didn't recently commit"
  echo    " on this branch."
  echo "To format all files: $0 -a"
  exit 1
fi

# join filename extensions with the word 'and'
extensions=$(
  sort -u <<< "${files[*]/#*./.}" | paste -sd ' ' - | sed 's/ / and /g'
)
printf "Formatting %s %s file%s\n" \
  "${#files[@]}" "$extensions" "$([[ ${#files[@]} -eq 1 ]] || echo s)"

for f in "${files[@]}"; do
  clang-format -i -style=file "$f"
  echo -n .
done
echo
echo "Done"
