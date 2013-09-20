<?php

/**
 * @file
 * Admin callbacks for Islandora Calliope Integration
 */

/**
 * Creates the admin form for islandora_calliope.
 *
 * @return array
 *   admin form.
 */
function islandora_calliope_admin($form, &$form_state) {
  
  //TODO  use state to set checkmark
  $form['islandora_calliope_url'] = array(
    '#type' => 'textfield',
    '#default_value' => variable_get('islandora_calliope_url','localhost:8080/calliope'),
    '#description' => t('URL of Calliope installation'),
  );
  return $form;
}