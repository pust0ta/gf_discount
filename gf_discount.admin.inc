<?php

/**
 * @file
 * Функции, вызываемые на страницах административного управления модулем.
 *
 * @see gf_discount_menu()
 */

function gf_discount_settings_form(){
  $form = [];
  $defaults = [ // сумма в тыс. руб. => скидка(в процентах)
    50 => 5, 
    100 => 10
  ];
  $settings = variable_get('gf_discount_newbie_discounts', $defaults);
  $levels = range(1, count($settings));

  $form['newbie_discount'] = [
    '#type' => 'fieldset',
    '#title' => t('Newbie discounts'),
    '#description' => t('Setting discounts for a newbie.'),
  ];

  $i = 0;
  foreach($levels as $level) {
    $form['newbie_discount']['level_'. $level] = [
      '#type' => 'fieldset',
      '#title' => t('Level %level Discount', ['%level' => $level]),
      '#description' => t('Discount for the level %level', ['%level' => $level]),
    ];
    // Настройка суммы для уровня.
    $form['newbie_discount']['level_' . $level]['newbie_level_' . $level . '_sum'] = [
      '#type' => 'textfield',
      '#title' => t('Required amount (in thousands of rubles)'),
      '#description' => t('The minimum order amount required to receive a discount.'),
      '#default_value' => array_keys($settings)[$i],
      '#required' => true,
      '#element_validate' => array('element_validate_integer_positive'), 
    ];
    // Настройка скидки для указанной выше суммы.
    $form['newbie_discount']['level_' . $level]['newbie_level_' . $level . '_discount'] = [
      '#type' => 'textfield',
      '#title' => t('Discount (in percent)'),
      '#description' => t(''),
      '#default_value' => array_values($settings)[$i],
      '#required' => true,
      '#element_validate' => array(
        'element_validate_integer_positive', 
        'gf_discount_percent_validate'
      ), 
      '#maxlength' => 3,
    ];

    $i++;
  }
  $form['#newbie_discount_levels'] = $levels;

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];

  return $form;
}

/**
 * Implements hook_form_submit()
 *
 * Собираем настройки по дисконтам для новичков в один массив.
 */
function gf_discount_settings_form_submit($form, &$form_state){
  $levels = $form['#newbie_discount_levels'];
  $values = $form_state['values'];
  $settings = [];
  foreach ($levels as $level) {
    $settings[$values['newbie_level_'. $level . '_sum']] = $values['newbie_level_' . $level . '_discount'];
  }
  variable_set('gf_discount_newbie_discounts', $settings);
}

/**
 * Проверка на правильный ввод процентов.
 *
 */
function gf_discount_percent_validate($element, $form_state) {
  if ($element['#value'] > 100) {
    form_error($element, t('The value must be less than or equal to 100.'));
  }
}


/**
 * Форма для настройки скидок для нужных ролей.
 *
 */
function gf_discount_roles_settings_form() {
  $form = [];
  $settings = variable_get('gf_discount_role_discounts', []);

  $form['summarize_discounts'] = [
    '#type' => 'checkbox',
    '#title' => t('Summarize discounts'),
    '#description' => t('Summarize discounts if the user has several roles to which different discounts apply. If the check box is not checked, the maximum discount will be selected.'),
    '#default_value' => variable_get('gf_discount_summarize_discounts', FALSE),
  ];

  $form['discount_by_role'] = [
    '#type' => 'fieldset',
    '#title' => t('Discount by role'),
    '#description' => t('Setting up discounts for custom roles')
  ];

  $roles = user_roles(TRUE);
  ksort($roles);
  foreach($roles as $rid => $role) {
    $form['discount_by_role']['role_' . $rid] = [
        '#type' => 'textfield',
        '#title' => $role . ' (' . $rid . ')',
        '#description' => t('Set the discount in percent.'),
        '#default_value' => isset($settings[$rid]) ? $settings[$rid] : 0,
        '#element_validate' => array(
          'element_validate_integer', 
          'gf_discount_percent_validate'
        ), 
        '#maxlength' => 3,
        '#width' => 4,
    ];
  }

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];

  return $form;
}


/**
 * Сохранение настроек по скидкам для ролей.
 *
 */
function gf_discount_roles_settings_form_submit($form, &$form_state) {
  $discounts = [];
  foreach($form_state['values'] as $key => $val) {
    if ($key == 'summarize_discounts') {
      variable_set('gf_discount_summarize_discounts', $val);
    }
    elseif (substr($key, 0, 5) === 'role_' && $val > 0) {
      $role_id = (int) str_replace('role_', '', $key);
      $discounts[$role_id] = $val;
    }
  }

  variable_set('gf_discount_role_discounts', $discounts);
}


/**
 * Настройка скидок на основе суммы по заказам за прошлый месяц.
 *
 */
function gf_discount_montly_settings_form($form, &$form_state){
  $form['#tree'] = TRUE;
  $form['roles_expire'] = array(
    '#title' => t('Roles expire'),
    '#type' => 'textfield',
    '#size' => 10,
    '#default_value' => variable_get('gf_discount_roles_expire', '1 month'),
    '#maxlength' => 32,
    '#required' => true,
    '#attributes' => array('class' => array('role-expire-role-expiry')),
    '#description' => t('Enter the time span you want to set as the default duration for a roles. Examples: 12 hours, 1 day, 3 days, 4 weeks, 3 months, 1 year. This value may be any !l-compatible relative form.',
      array('!l' => l('strtotime', 'http://php.net/manual/en/function.strtotime.php'))
    )
  );
  $form['sum_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Assigning a role depending on the amount spent in the past month.'),
    '#prefix' => '<div id="sum-fieldset-wrapper">',
    '#suffix' => '</div>',
  );

  $settings = variable_get('gf_discount_montly_sum_roles_discount', []);

  if (empty($form_state['num_options'])) {
    $form_state['num_options'] = count($settings) ?: 1;
  }

  for ($i = 0; $i < $form_state['num_options']; $i++) {
    // Парсим сохраненные настройки для получения 
    // дефолтных занчений для полей.
    $settings_values = array_values($settings);
    $settings_keys = array_keys($settings);
    list($min, $max) = isset($settings_keys[$i]) ? explode(':', $settings_keys[$i]) : [];
    $role_id = $settings_values[$i] ?? '';

    $form['sum_fieldset'][$i] = array(
      '#type' => 'fieldset',
    );
    $form['sum_fieldset'][$i]['min'] = array(
      '#type' => 'textfield',
      '#title' => t('MIN Sum'),
      '#description' => t('Minimum sum of the range'),
      '#element_validate' => array('element_validate_integer_positive'), 
      '#default_value' => $min,
    );
    $form['sum_fieldset'][$i]['max'] = array(
      '#type' => 'textfield',
      '#title' => t('MAX Sum'),
      '#description' => t('Maximum sum of the range'),
      '#element_validate' => array('element_validate_integer_positive'), 
      '#default_value' => $max,
    );
    $form['sum_fieldset'][$i]['role_id'] = array(
      '#type' => 'select',
      '#title' => t('Role'),
      '#options' => user_roles(TRUE),
      '#description' => t('The role must be assigned when it enters the range.'),
      '#default_value' => $role_id,
    );
  }

  $form['add_more'] = array(
    '#type' => 'submit',
    '#value' => t('Add one more'),
    '#validate' => [],
    '#submit' => array(
      'gf_discount_montly_ajax_add_one',
    ),
    '#ajax' => array(
      'callback' => 'gf_discount_montly_add_more_ajax',
      'wrapper' => 'sum-fieldset-wrapper',
    ),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Implements hook_form_submit()
 *
 */
function gf_discount_montly_settings_form_submit($form, &$form_state) {
  $settings = [];
  $values = $form_state['values']['sum_fieldset'];
  foreach ($values as $num => $val) {
    if (!$val['min'] || !$val['max']) continue;
    // Ключ - строка, составленная из мин. и макс. сумм, разделенных
    // двоеточием.
    // Значение = ID роли.
    $key = sprintf('%s:%s', $val['min'], $val['max']);
    $settings[$key] = $val['role_id'];
  }

  ksort($settings, SORT_NUMERIC);
  variable_set('gf_discount_montly_sum_roles_discount', $settings);
}

/**
 * Implements hook_form_validate()
 *
 */
function gf_discount_montly_settings_form_validate($form, &$form_state) {
  $values = $form_state['values']['sum_fieldset'];
  $role_ids = [];

  $roles_expire = $form_state['values']['roles_expire'];
  if(strtotime($roles_expire)) {
    variable_set('gf_discount_roles_expire', $roles_expire);
  }
  else {
    form_set_error('roles_expire', t('Incorrect expiration date for roles. See !l', 
      array('!l' => l('examples', 'http://php.net/manual/en/function.strtotime.php#refsect1-function.strtotime-examples'))));
  }

  foreach ($values as $num => $val) {
    // Незаполненные сеты не учитываем.
    if (!$val['min'] || !$val['max']) continue;

    // Проверка на то, что заданная минимальная сумма реально 
    // меньше заданной максимальной.
    if ((int) $val['min'] >=  (int) $val['max']) {
      form_set_error('sum_fieldset][' . $num, t('The max amount must be greater than the min.'));
    }
    // Проверяем, что одна и та же роль не была указана дважды.
    if (in_array($val['role_id'], $role_ids)) {
      form_set_error('sum_fieldset][' . $num . '][role_id', t('The role assigned twice.'));
    }
    else {
      $role_ids[] = $val['role_id'];
    }
  }
}

/**
 * Возвращаем аяксом обновленный формсет.
 *
 */
function gf_discount_montly_add_more_ajax($form, &$form_state){
  return $form['sum_fieldset'];
}

/**
 * Добавление сета с диапазоном.
 *
 */
function gf_discount_montly_ajax_add_one($form, &$form_state) {
  // Увеличиваем счетчик опций.
  $form_state['num_options']++;
  $form_state['rebuild'] = TRUE;
}

