Import("env")

from pathlib import Path


project_src_dir = Path(env.subst("$PROJECT_SRC_DIR")).resolve()


def keep_project_warnings(build_env, node):
    source_path = Path(node.srcnode().get_abspath()).resolve()
    try:
        source_path.relative_to(project_src_dir)
        return node
    except ValueError:
        filtered_env = build_env.Clone()
        filtered_env.AppendUnique(CCFLAGS=["-w"])
        return filtered_env.Object(node)[0]


env.AddBuildMiddleware(keep_project_warnings)
