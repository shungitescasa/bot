from lupa import LuaRuntime
from jinja2 import Environment, FileSystemLoader
from pathlib import Path
import markdown

lua = LuaRuntime(unpack_returned_tuples=True)
env = Environment(loader=FileSystemLoader("www/partials"))

template = env.get_template("luascript.shtml")

out_dir = Path("www/commands")
out_dir.mkdir(parents=True, exist_ok=True)

commands = []

# generating lua pages
for lua_file in Path("luascripts").glob("*.lua"):
    data = lua.execute(lua_file.read_text())
    item = {
        "name": data["name"],
        "delay": data["delay_sec"],
        "summary": data["summary"] if "summary" in data else "N/A",
        "description": data["description"] if "description" in data else "*No description*",
        "subcommands": ["-"],
        "aliases": ["-"],
        "minimal_rights": data["minimal_rights"]
    }

    item["description"] = markdown.markdown(item["description"])

    if "subcommands" in data and len(data["subcommands"]) > 0:
        item["subcommands"] = [f"<code>{data["subcommands"][c]}</code>" for c in data["subcommands"]]
    
    if "aliases" in data and len(data["aliases"]) > 0:
        item["aliases"] = [f"<code>{data["aliases"][c]}</code>" for c in data["aliases"]]
    
    commands.append(item)

    html = template.render(**item)
    (out_dir / f"{lua_file.stem}.html").write_text(html)

# generating index
index = env.get_template("commands.shtml").render(commands=commands)
(out_dir / "index.shtml").write_text(index)