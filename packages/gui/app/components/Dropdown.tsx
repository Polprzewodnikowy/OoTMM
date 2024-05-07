import React from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faQuestionCircle } from '@fortawesome/free-solid-svg-icons';

import { Group } from './Group';
import { Text } from './Text';
import { Tooltip } from './Tooltip';

type DropdownProps = {
  label?: string;
  options: { value: string; name: string }[];
  value: string;
  tooltip?: React.ReactNode;
  onChange: (value: string) => void;
}
export const Dropdown = ({ label, options, value, tooltip, onChange }: DropdownProps) => {
  return (
    <label>
      <Group direction='vertical'>
        <Group direction='horizontal'>
          <Text size='xl' className="label-main">{label}</Text>
          {tooltip && <Tooltip>{tooltip}</Tooltip>}
        </Group>
        <select value={value} onChange={(e) => onChange(e.target.value)}>
          {options.map((option) => (
            <option key={option.value} value={option.value}>
              {option.name}
            </option>
          ))}
        </select>
      </Group>
    </label>
  );
};
