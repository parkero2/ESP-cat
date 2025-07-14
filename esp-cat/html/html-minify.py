from htmlmin import minify as html_minify
from rcssmin import cssmin
from rjsmin import jsmin
import re

def minify_html_with_assets(html):
    # Minify any embedded CSS
    html = re.sub(r'(<style.*?>)(.*?)(</style>)',
                  lambda m: f"{m.group(1)}{cssmin(m.group(2))}{m.group(3)}",
                  html,
                  flags=re.DOTALL)

    # Minify any embedded JS
    html = re.sub(r'(<script.*?>)(.*?)(</script>)',
                  lambda m: f"{m.group(1)}{jsmin(m.group(2))}{m.group(3)}",
                  html,
                  flags=re.DOTALL)

    # Minify the final HTML
    return html_minify(html, remove_comments=True, remove_empty_space=True)

def main():
    with open('index.html', 'r', encoding='utf-8') as f:
        html = f.read()

    minified = minify_html_with_assets(html)

    with open('index-min.html', 'w', encoding='utf-8') as f:
        f.write(minified)

if __name__ == '__main__':
    main()
