# BLACKBOX PS4 — portage natif

Cette branche prépare un portage natif PS4 indépendant du binaire PS5. Le code PS5 et les catalogues historiques à la racine ne sont pas modifiés.

## Modules ajoutés

- core/include/blackbox/catalog_entry.hpp : modèle de métadonnées d'une entrée.
- core/include/blackbox/catalog_repository.hpp et core/src/catalog_repository.cpp : lecture et validation minimale du JSON.
- core/include/blackbox/catalog_filter.hpp et core/src/catalog_filter.cpp : recherche par titre, identifiant et région.
- core/tests/catalog_tests.cpp : tests du parseur et des filtres.
- CMakeLists.txt : compilation portable du noyau.
- .github/workflows/ps4-core-tests.yml : compilation et tests automatiques GitHub Actions.

## Règles de compatibilité

Le parseur ne conserve que les métadonnées PS4 : plateforme, titre, Title ID, région, version, taille et visuels. Il ignore volontairement les champs d'URL de paquets et de sources distantes. Il ne télécharge, ne distribue et n'installe aucun jeu.

Une entrée PS5 n'est jamais convertie en entrée PS4. Un Title ID ou une jaquette ne prouve pas qu'un paquet est compatible.

## Compilation du noyau

Prérequis : CMake 3.16+, compilateur C++17 et accès réseau au premier téléchargement de la dépendance JSON.

    cmake -S ps4-port -B build/ps4-core -DCMAKE_BUILD_TYPE=Release
    cmake --build build/ps4-core --parallel
    ctest --test-dir build/ps4-core --output-on-failure

## État du portage

Le noyau portable et ses tests constituent la première couche, pas encore une application PS4. Le dépôt ne contient actuellement ni source native PS4 d'origine, ni toolchain PS4/OpenOrbis configurée, ni cible FPKG vérifiée. Les adaptateurs PS4 (contrôleur, rendu, stockage, réseau et packaging) seront ajoutés séparément après validation de l'environnement. Aucun faux PKG/FPKG ne sera publié.
