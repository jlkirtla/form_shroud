<?php

/**
 * @file
 * Provides forms for managing form shroud registry.
 */

/**
 * Form constructor for adding an entry to the form shroud registry.
 */
function form_shroud_settings_define(&$form) {
  $shroud_registry = db_select('form_shroud_registry', 'f')
    ->fields('f')
    ->execute()
    ->fetchAllAssoc('nid');
  if (!user_access('form_shroud bypass')) {
    drupal_access_denied();
  }
  else {
    $form['form_shroud_add'] = array(
      '#type' => 'textfield',
      '#title' => t('Add Node to Registry'),
      '#default_value' => '',
      '#size' => 60,
      '#description' => 'NIDs or Titles can be used here.',
      '#maxlength' => 120,
      '#autocomplete_path' => 'formshroud/input',
    );
    $form['form_shroud_submit_add'] = array(
      '#type' => 'submit',
      '#value' => 'Add',
    );
  }
  $registry_table = array(
    'header' => array('NID', 'Title', 'Path', 'Operations'),
    'attributes' => array(),
    'caption' => '',
    'colgroups' => array(),
    'sticky' => '',
    'empty' => '',
    'rows' => array(),
  );
  foreach ($shroud_registry as $row) {
    $registry_table['rows'][$row->nid] = array(
      $row->nid,
      $row->title,
      l($row->path, $row->path),
      l(t('Edit'), 'admin/structure/formshroud/' . $row->nid . '/edit'),
    );
  }
  $form['form_shroud_registry_table'] = array(
    '#markup' => theme_table($registry_table),
  );
  return $form;
}

/**
 * Form submission handler for uform_shroud_settings_define().
 */
function form_shroud_settings_define_submit(&$form, $form_state) {
  if (!empty($form_state['values']['form_shroud_add'])) {
    $node = db_select('node', 'n')
      ->fields('n')
      ->condition(db_or()->condition('title', $form_state['values']['form_shroud_add'])
      ->condition('nid', $form_state['values']['form_shroud_add']))
      ->execute()
      ->fetchAssoc('nid');
    $check = db_select('form_shroud_registry', 'f')
      ->fields('f')
      ->condition('nid', $node['nid'])
      ->execute()
      ->fetchAssoc();
    if (empty($check)) {
      $source = 'node/' . $node['nid'];
      $path = ($alias = db_select('url_alias', 'u')
        ->fields('u')
        ->condition('source', $source)
        ->execute()
        ->fetchAssoc()
      ) ? $alias : $source;
      db_insert('form_shroud_registry')
        ->fields(array(
          'nid' => $node['nid'],
          'title' => $node['title'],
          'path' => $path['alias'],
        ))
        ->execute();
      $message = '"' . $node['title'] . '" ' . t('has been added to the registry.');
      drupal_set_message(check_plain($message));
    }
    else {
      $message = '"' . $node['title'] . '" ' . t('is already in the registry, or an error has occurred.');
      drupal_set_message(check_plain($message), 'warning');
    }
  }
}

/**
 * Form constructor for the removal of an entry from the form shroud registry.
 */
function form_shroud_settings_remove(&$form) {
  $shroud_registry = db_select('form_shroud_registry', 'f')
    ->fields('f')
    ->execute()
    ->fetchAllAssoc('nid');
  if (!empty($shroud_registry)) {
    foreach ($shroud_registry as $var) {
      $options[$var->nid] = $var->title . ' (NID: ' . $var->nid . ') (PATH: ' . l($var->path, $var->path) . ')';
    }
  }
  else {
    $options = FALSE;
  }
  if (!user_access('form_shroud bypass')) {
    drupal_access_denied();
  }
  if ($options) {
    $form['form_shroud_list'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Remove Nodes from Registry'),
      '#options' => $options,
    );
    $form['form_shroud_submit_remove'] = array(
      '#type' => 'submit',
      '#value' => 'Remove',
    );
  }
  else {
    drupal_set_message(t('The registry is currently empty.'));
  }
  return $form;
}

/**
 * Form submission handler for form_shroud_settings_remove().
 */
function form_shroud_settings_remove_submit(&$form, $form_state) {
  if (isset($form_state['values']['form_shroud_list'])) {
    foreach ($form_state['values']['form_shroud_list'] as $nid => $title) {
      if ($title != 0) {
        $num_rows[] = db_delete('form_shroud_registry')
          ->condition('nid', $nid)
          ->execute();
      }
    }
    if (isset($num_rows)) {
      $drupal_message = count($num_rows) . ' ' . t('entries have been removed from the registry.');
      drupal_set_message(check_plain($drupal_message));
    }
  }
}

/**
 * Form constructor for editing a node's shrouded form elements.
 *
 * @param int $nid
 *   Node ID to indicate which node's form properties is edited.
 */
function form_shroud_settings_edit(&$form, $form_state, $nid) {
  $elements = module_invoke_all('shroud_define');
  $shroud_registry = db_select('form_shroud_registry', 'f')->fields('f')->condition('nid', $nid)->execute()->fetchAssoc();
  $form['title'] = array(
    '#markup' => '<h2>Title: ' . $shroud_registry['title'] . '</h2><h2>NID: ' . $shroud_registry['nid'] . '</h2><h2>Path: ' . l($shroud_registry['path'], $shroud_registry['path']) . '</h2>',
  );
  $form['field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Define (check) properties to be shrouded.'),
  );
  $reg = unserialize($shroud_registry['settings']);
  foreach ($elements as $source => $options) {
    foreach ($options as $machine_name => $title) {
      if (is_array($title)) {
        foreach ($title as $generic => $variable) {
          $form['field'][$generic] = array(
            '#type' => 'checkbox',
            '#title' => t($variable),
            '#default_value' => (isset($reg[$machine_name][$generic])) ? 1 : 0,
          );
        }
      }
      else {
        $form['field'][$machine_name] = array(
          '#type' => 'checkbox',
          '#title' => t($title),
          '#default_value' => (isset($reg[$source][$machine_name])) ? 1 : 0,
        );
      }
    }
  }
  $form['nid'] = array(
    '#type' => 'hidden',
    '#value' => $nid,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );
  return $form;
}

/**
 * Form submission handler for form_shroud_settings_edit().
 */
function form_shroud_settings_edit_submit(&$form, $form_state) {
  $elements = module_invoke_all('shroud_define', array());
  foreach ($elements as $key => $val) {
    foreach ($val as $name => $value) {
      if (is_array($value)) {
        foreach ($value as $gen => $var) {
          $insert[$name][$gen] = 1;
        }
      }
      else {
        if ($form_state['values'][$name]) {
          $insert[$key][$name] = 1;
        }
      }
    }
  }
  drupal_set_message(t('The form elements have been shrouded.'));
  db_update('form_shroud_registry')->fields(array('settings' => serialize($insert)))->condition('nid', $form_state['values']['nid'])->execute();
}

/**
 * Autocomplete menu callback function.
 */
function form_shroud_json_callback($string) {
  $matches = array();
  $list = db_select('node', 'n')->fields('n')->condition('n.title', '%' . db_like($string) . '%', 'LIKE')->range(0, 10)->execute();
  foreach ($list as $row) {
    $matches[$row->title] = $row->title . ' (NID: ' . $row->nid . ')';
  }
  drupal_json_output($matches);
}
