phosh_search_sources = [
  'search/search-result.c',
  'search/search-result.h',
  'search/search-result-app-frame.c',
  'search/search-result-app-frame.h',
  'search/search-result-content.c',
  'search/search-result-content.h',
  'search/search-result-frame.c',
  'search/search-result-frame.h',
  'search/search-result-list.c',
  'search/search-result-list.h',
  'search/search-result-source.c',
  'search/search-result-source.h',
]

phoshsearch_sources = [
  'search-client.c',
  'search-client.h',
  'search-result-meta.c',
  'search-result-meta.h',
  'search-source.c',
  'search-source.h',
  generated_search_dbus_sources,
]

phoshsearch_deps = [
  gio_dep,
  gio_unix_dep,
  dependency('gdk-pixbuf-2.0'),
]

phoshsearch_lib = library('phoshsearch', phoshsearch_sources,
                          dependencies: phoshsearch_deps,
                          include_directories: [phosh_dbus_inc],
                          install_dir: pkglibdir,
                          install: true)
phoshsearch_inc = include_directories('.')

phoshsearch_dep = declare_dependency(sources: generated_search_dbus_sources,
                                     include_directories: [phosh_dbus_inc, phoshsearch_inc],
                                     link_with: phoshsearch_lib,
                                     dependencies: phoshsearch_deps)
