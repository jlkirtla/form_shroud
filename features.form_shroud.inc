<?php

/**
 * @file
 * Provides features export and import functionality.
 */

/**
 * Implements hook_features_export_options().
 */
function form_shroud_features_export_options() {
  $options = array();
  $result = db_select('form_shroud_registry', 'f')
    ->fields('f')
    ->execute()
    ->fetchAllAssoc('uid', PDO::FETCH_ASSOC);
  foreach ($result as $fsr) {
    $options[$fsr['uid']] = $fsr['title'];
  }
  return $options;
}

/**
 * Implements hook_features_export_render().
 */
function form_shroud_features_export_render($module, $data, $export = NULL) {
  $code = array();
  $code[] = '  $shroud = array();';
  $code[] = '';

  foreach ($data as $uid) {
    $row = db_select('form_shroud_registry', 'f')
      ->fields('f', array('form_id', 'title', 'settings', 'uid'))
      ->condition('uuid', $uid)
      ->execute()
      ->fetchAssoc(PDO::FETCH_ASSOC);
    $export = features_var_export($row, '  ');
    $code[] = "  // Exported shroud item: {$uid}";
    $code[] = "  \$shroud['{$uid}'] = {$export};";
  }
  $code[] = '';
  $code[] = '  return $shroud;';
  $code = implode("\n", $code);
  return array('form_shroud_registry_default_form_shroud' => $code);
}

/**
 * Implements hook_features_export().
 */
function form_shroud_features_export($data, &$export, $module_name = '') {
  $pipe = array();
  // Default hooks are provided by the feature module so we need to add
  // it as a dependency.
  $export['dependencies']['features'] = 'features';
  $export['dependencies']['form_shroud'] = 'form_shroud';
  $export['dependencies']['uuid'] = 'uuid';

  foreach ($data as $value) {
    $export['features']['form_shroud'][$value] = $value;
  }

  return $pipe;
}

/**
 * Implements hook_features_export_revert().
 */
function form_shroud_features_revert($module) {
  form_shroud_features_rebuild($module);
}

/**
 * Implements hook_features_export_rebuild().
 */
function form_shroud_features_rebuild($module) {
  if ($defaults = features_get_default('form_shroud', $module)) {
    foreach ($defaults as $uid => $shroud) {
      $check = db_select('form_shroud_registry', 'f')
        ->fields('f')
        ->condition('uid', $uid)
        ->execute()
        ->fetchAssoc(PDO::FETCH_ASSOC);
      if (!isset($check['fsid'])) {
        db_insert('form_shroud_registry')
          ->fields(array(
            'title' => $shroud['title'],
            'form_id' => $shroud['form_id'],
            'settings' => $shroud['settings'],
            'uid' => $shroud['uid'],
            ))
          ->execute();
      }
      else {
        db_update('form_shroud_registry')
          ->fields(array(
            'title' => $shroud['title'],
            'form_id' => $shroud['form_id'],
            'settings' => $shroud['settings'],
            'uid' => $shroud['uid'],
          ))
          ->condition('uid', $uid)
          ->execute();
      }
    }
  }
}
