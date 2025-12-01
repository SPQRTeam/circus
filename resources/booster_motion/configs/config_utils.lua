-- compare ser version
-- -1 for version1 < version2
-- 0 for version1 == version2
-- 1 for version1 > version2
local function compare_semver(version1, version2)
  local function split_version(version)
      local major, minor, patch = version:match("^(%d+)%.(%d+)%.(%d+)$")
      return tonumber(major), tonumber(minor), tonumber(patch)
  end

  local major1, minor1, patch1 = split_version(version1)
  local major2, minor2, patch2 = split_version(version2)

  if major1 < major2 then
      return -1
  elseif major1 > major2 then
      return 1
  else
      if minor1 < minor2 then
          return -1
      elseif minor1 > minor2 then
          return 1
      else
          if patch1 < patch2 then
              return -1
          elseif patch1 > patch2 then
              return 1
          else
              return 0
          end
      end
  end
end

-- read file content
local function read_file(file_path)
  local file = io.open(file_path, "r")
  if not file then
      return nil, "cannot open file: " .. file_path
  end

  local content = file:read("*all")
  file:close()
  return content
end

-- extract_model_version
local function extract_model_version(content)
  local model_version = content:match("Model Version:%s*(%S+)%s*")
  return model_version
end

local function get_version(default_version, is_real_bot)
  local version = default_version

  if is_real_bot then
    local file_path = "/opt/booster/robot_info.txt"
    local content, err = read_file(file_path)
    if not content then
        print(err)
    end

    local model_version = extract_model_version(content)
    if model_version then
        version = model_version
    else
        print("Model Version not not, use default version: " ..version)
    end
  end
  return version
end

-- default version is used when the real robot is not available
function VersionGreaterOrEquals(default_version, target_version, is_real_bot)
  local base_version = get_version(default_version, is_real_bot)
  return compare_semver(base_version, target_version) >= 0
end
