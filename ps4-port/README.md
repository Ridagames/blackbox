# BLACKBOX PS4 — portage en cours

Cette branche isole le travail de portage PS4 sans modifier le catalogue original.

## État initial
- `LIVE/catalog.json` est conservé à son emplacement et n'est pas réécrit.
- `ps4-port/index.html` est le premier prototype de navigateur de catalogue.
- Le prototype lit les métadonnées du catalogue, filtre strictement les entrées dont `platform` vaut `ps4`, et n'expose pas les URLs de releases.
- Les entrées identifiées PS5 ne sont jamais présentées comme exécutables sur PS4.

## Limites actuelles
Ce prototype est une interface web de validation, pas encore un exécutable PS4 ni un FPKG. Le binaire PS5 `blackbox.elf` et son conteneur `.ffpkg` ne peuvent pas être renommés ou convertis en FPKG PS4 : le code doit être recompilé et adapté à l'environnement PS4 (toolchain, APIs, contrôleur, stockage, réseau et packaging).

## Prochaines étapes
1. Identifier les sources natives et dépendances réellement disponibles dans le projet amont.
2. Définir une couche de données indépendante de la plateforme et conserver les métadonnées du catalogue.
3. Implémenter une interface native PS4 et un workflow de compilation OpenOrbis vérifiable.
4. Produire un FPKG de test seulement après validation d'un ELF PS4 fonctionnel.

Le prototype n'intègre pas de distribution de copies de jeux commerciaux. Il peut servir à afficher les fiches et métadonnées de contenus dont l'utilisateur détient les droits.
