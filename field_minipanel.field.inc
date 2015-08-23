<?php

/**
 * Implements hook_field_extra_fields().
 */
function field_minipanel_field_extra_fields() {
  // @todo taxonomy and user.
  foreach (array('node') as $entity_type) {
    $entity_info = entity_get_info($entity_type);
    if (empty($entity_info['bundles'])) {
      continue;
    }
    foreach ($entity_info['bundles'] as $bundle_name => $bundle) {
      $extra[$entity_type][$bundle_name]['display']['minipanel'] = array(
        'label' => t('Mini panel'),
        'description' => t('Display a mini panel'),
        'weight' => 0,
      );
    }
  }
  return $extra;
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function field_minipanel_form_field_ui_display_overview_form_alter(&$form, &$form_state) {
  $entity_type = $form['#entity_type'];
  $bundle = $form['#bundle'];
  $view_mode = $form['#view_mode'];

  $extra_fields = _field_minipanel_get_fields($entity_type, $bundle);
  $extra_fields = array_intersect($extra_fields, $form['#extra']);
  if (empty($extra_fields)) {
    return;
  }

  // Generate a list of all mini panels.
  $minipanels_all = panels_mini_load_all();
  $minipanels_list = array('none' => t('None'));
  foreach ($minipanels_all as $minipanel) {
    if (empty($minipanel->disabled)) {
      $minipanels_list[$minipanel->name] = $minipanel->admin_title;
    }
  }

  // Add an instance of each minipanel field to the field UI.
  foreach ($extra_fields as $field_name) {
    $minipanel = NULL;
    // Use the form value, in case it was modified.
    if (isset($form_state['values']['fields'][$field_name]['settings_edit_form']['settings']['minipanel_name'])) {
      $minipanel_id = $form_state['values']['fields'][$field_name]['settings_edit_form']['settings']['minipanel_name'];
      if ($minipanel_id && $minipanel_id != 'none') {
        $minipanel = panels_mini_load($minipanel_id);
      }
    }
    // Default to loeading the currently saved value
    else {
      $minipanel = field_minipanel_load_minipanel($field_name, $entity_type, $bundle, $view_mode);
    }
    $minipanel_name = is_object($minipanel) ? $minipanel->name : 'none';
    $minipanel_title = is_object($minipanel) ? $minipanel->admin_title : t('none');

    // Base button element for the various formatter settings actions.
    $base_button = array(
      '#submit' => array('field_ui_display_overview_multistep_submit'),
      '#ajax' => array(
        'callback' => 'field_ui_display_overview_multistep_js',
        'wrapper' => 'field-display-overview-wrapper',
        'effect' => 'fade',
      ),
      '#field_name' => $field_name,
    );

    // Render the edit form.
    if ($form_state['formatter_settings_edit'] == $field_name) {
      // We are currently editing this field's formatter settings. Display the
      // settings form and submit buttons.
      $form['fields'][$field_name]['format']['settings_edit_form'] = array();

      // Render the settings form.
      $form['fields'][$field_name]['format']['#cell_attributes'] = array('colspan' => 3);
      $form['fields'][$field_name]['format']['settings_edit_form'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('field-formatter-settings-edit-form')),
        '#parents' => array('fields', $field_name, 'settings_edit_form'),
        'settings' => array(
          'minipanel_name' => array(
            '#type' => 'select',
            '#title' => t('Minipanel'),
            '#description' => t('Render the following mini panel as this field.'),
            '#options' => $minipanels_list,
            '#default_value' => $minipanel_name,
          ),
        ),
        'actions' => array(
          '#type' => 'actions',
          'save_settings' => $base_button + array(
            '#type' => 'submit',
            '#name' => $field_name . '_formatter_settings_update',
            '#value' => t('Update'),
            '#op' => 'update',
          ),
          'cancel_settings' => $base_button + array(
            '#type' => 'submit',
            '#name' => $field_name . '_formatter_settings_cancel',
            '#value' => t('Cancel'),
            '#op' => 'cancel',
            // Do not check errors for the 'Cancel' button, but make sure we
            // get the value of the 'formatter type' select.
            '#limit_validation_errors' => array(array('fields', $field_name, 'type')),
          ),
        ),
      );
      $form['fields'][$field_name]['#attributes']['class'][] = 'field-formatter-settings-editing';
    }
    // Render the summary
    else {
      $form['fields'][$field_name]['settings_summary'] = array(
        '#markup' => '<div class="field-formatter-summary">' . t('Minipanel: @minipanel', array('@minipanel' => $minipanel_title)) . '</div>',
        '#cell_attributes' => array('class' => array('field-formatter-summary-cell')),
      );
      $form['fields'][$field_name]['settings_edit'] = array(
        '#submit' => array('field_ui_display_overview_multistep_submit'),
        '#ajax' => array(
          'callback' => 'field_ui_display_overview_multistep_js',
          'wrapper' => 'field-display-overview-wrapper',
          'effect' => 'fade',
        ),
        '#field_name' => $field_name,
        '#type' => 'image_button',
        '#name' => $field_name . '_formatter_settings_edit',
        '#src' => 'misc/configure.png',
        '#attributes' => array('class' => array('field-formatter-settings-edit'), 'alt' => t('Edit')),
        '#op' => 'edit',
        '#limit_validation_errors' => array(array('fields', $field_name, 'type')),
        '#prefix' => '<div class="field-formatter-settings-edit-wrapper">',
        '#suffix' => '</div>',
      );
    }
  }
  $form['#submit'][] = 'field_minipanel_extra_field_minipanel_submit';
}

/**
 * Submit handler; Save the minipanel naem for a speciic display setup.
 */
function field_minipanel_extra_field_minipanel_submit(&$form, &$form_state) {
  $entity_type = $form['#entity_type'];
  $bundle = $form['#bundle'];
  $view_mode = $form['#view_mode'];
  $extra_fields = _field_minipanel_get_fields($entity_type, $bundle);
  foreach ($extra_fields as $field_name) {
    if (!empty($form_state['formatter_settings'][$field_name]['minipanel_name'])) {
      $minipanel_key = "$field_name:$entity_type:$bundle:$view_mode";
      $minipanel_name = $form_state['formatter_settings'][$field_name]['minipanel_name'];
      if ($minipanel_name == 'none') {
        $minipanel_name = NULL;
      }
      field_minipanel_set_config($minipanel_key, $minipanel_name);
      // Clear all view modes and all entities
      cache_clear_all("$entity_type:$bundle", 'cache_field_minipanel');
    }
  }
}
