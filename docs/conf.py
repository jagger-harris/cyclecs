"""Sphinx config for the Cyclecs documentation."""

from pathlib import Path

# Project information
project = "Cyclecs"
author = "Jagger Harris"
copyright = "2026, Jagger Harris"

# Paths
DOCS_DIR = Path(__file__).parent
DOXYGEN_XML = DOCS_DIR.parent / "doxygen" / "xml"

# Extensions
extensions = ["breathe"]

breathe_projects = {
    "cyclecs": str(DOXYGEN_XML),
}
breathe_default_project = "cyclecs"

# Treat C headers/sources as C rather than C++
breathe_domain_by_extension = {
    "c": "c",
    "h": "c",
}
breathe_default_members = ()

# General config
templates_path = ["_templates"]
exclude_patterns = [
    ".venv",
    "_build",
    "Thumbs.db",
    ".DS_Store",
]

primary_domain = "c"
highlight_language = "c"

# HTML output
html_theme = "sphinx_rtd_theme"
