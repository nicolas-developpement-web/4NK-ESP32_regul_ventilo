# 4NK-ESP32_regul_ventilo
Création d'un system ESP32 de régulation de vitesse de rotation d'un ventilateur

MODE DE FONCTIONNEMENT:

- Se connecter au point wifi (AP) qui vient de se créer

- Selectionner un bouton pour changer de mode de fonctionnement:

  - mode 0 = Manuel -> Action manuelle directe sur la vitesse du ventilateur
    - Action sur le curseur pour varier la vitesse

  - mode 1 = Chaudiere -> fait tourner le ventilo pour chauffer jusqu'à la temperature souhaitée
      - Click sur bouton + ou - pour la definition de la temperature souhaitée par rotation autonome du ventilateur

  - mode 2 = Climatiseur -> fait tourner le ventilo pour refroidir jusqu'a la temperature souhaitée
      - Click sur bouton + ou - pour la definition de la temperature souhaitée par rotation autonome du ventilateur

Le systeme sauvegarde automatiquement le mode et la temperature choisie
