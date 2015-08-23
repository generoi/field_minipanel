<?php

/**
 * Implements hook_commerce_cart_attributes_refresh_alter().
 *
 * Refresh the minipanel when a product attribute gets modified.
 * This is mostly a duplicate of field_minipanel_node_view(), but taking into
 * account the forms product id.
 *
 * This data cant be cached as eg price can depends on too many things.
 */
function field_minipanel_commerce_cart_attributes_refresh_alter(&$commands, $form, $form_state) {
  $entity_type = $form_state['context']['entity_type'];
  $entity_id = $form_state['context']['entity_id'];
  $view_mode = $form_state['context']['display'];
  $product_id = $form['product_id']['#value'];

  // The parent entity (product display).
  if (!($entity = entity_load($entity_type, array($entity_id)))) {
    return;
  }
  $entity = reset($entity);
  list(,, $bundle) = entity_extract_ids($entity_type, $entity);

  // Fetch the minipanels associated with the entity display.
  $minipanels = field_minipanel_get_minipanels($entity_type, $bundle, $view_mode, $product_id);
  $field_product = _field_minipanel_get_product_reference_field($entity_type, $bundle);
  ctools_include('context');
  foreach ($minipanels as $field_name => $minipanel) {
    $contexts = array();
    // Apply the required contexts.
    foreach ($minipanel->requiredcontexts as $requiredcontext) {
      switch ($requiredcontext['keyword']) {
        // Context type is the same as the entity type.
        case "$entity_type":
          $contexts[] = ctools_context_create("entity:$entity_type", $entity);
          break;

        // Support product reference contexts.
        case 'commerce_product':
          // Make sure the field has values in the correct language
          // @todo language
          $items = field_get_items($entity_type, $entity, $field_product);
          if (empty($items)) {
            continue;
          }
          // Find the product id used in the refresh.
          foreach ($items as $item) {
            if ($item['product_id'] == $product_id) {
              break;
            }
          }
          $product = entity_load('commerce_product', array($item['product_id']));
          if (!empty($product)) {
            $product = reset($product);
            $contexts[] = ctools_context_create('entity:commerce_product', $product);
          }
          break;
      }
    }
    $context = ctools_context_match_required_contexts($minipanel->requiredcontexts, $contexts);
    $minipanel->context = $minipanel->display->context = ctools_context_load_contexts($minipanel, FALSE, $context);
    $minipanel->display->css_id = panels_mini_get_id($minipanel->name);
    // Store the rendered output.
    $minipanels[$field_name] = panels_render_display($minipanel->display);
  }

  // Replace the current versions with the new ones.
  foreach ($minipanels as $field_name => $minipanel) {
    $css_class = 'field-minipanel-' . $field_name;
    $output = '<div class="' . $css_class . '">' . $minipanel . '</div>';
    $commands[] = ajax_command_replace('.' . $css_class, $output);
  }
}

